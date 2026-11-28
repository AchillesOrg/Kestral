#include "bundler.hpp"
#include "ast.hpp"
#include "error.hpp"
#include "parser.hpp"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace kestral {

namespace {

// Construct markers at runtime/joined literals so contiguous signatures don't appear in raw code bytes.
std::string magic_beg() {
    return std::string("KESTRAL_") + "APP_BEG!";
}

std::string magic_end() {
    return std::string("KESTRAL_") + "APP_END!";
}

constexpr std::size_t kMagicLen = 16;
constexpr std::size_t kFooterTailLen = 8 + kMagicLen; // u64 payload_len + MAGIC_END

void write_u32_le(std::string& out, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) {
        out.push_back(static_cast<char>((v >> (i * 8)) & 0xFF));
    }
}

void write_u64_le(std::string& out, std::uint64_t v) {
    for (int i = 0; i < 8; ++i) {
        out.push_back(static_cast<char>((v >> (i * 8)) & 0xFF));
    }
}

void write_str_u32(std::string& out, const std::string& s) {
    write_u32_le(out, static_cast<std::uint32_t>(s.size()));
    out.append(s);
}

bool read_u32_le(const std::string& buf, std::size_t& pos, std::uint32_t& v) {
    if (pos + 4 > buf.size()) return false;
    std::uint32_t res = 0;
    for (int i = 0; i < 4; ++i) {
        res |= (static_cast<std::uint32_t>(static_cast<unsigned char>(buf[pos + static_cast<std::size_t>(i)]))
                << (i * 8));
    }
    pos += 4;
    v = res;
    return true;
}

std::uint64_t decode_u64_le(const char* p) {
    std::uint64_t res = 0;
    for (int i = 0; i < 8; ++i) {
        res |= (static_cast<std::uint64_t>(static_cast<unsigned char>(p[i])) << (i * 8));
    }
    return res;
}

bool read_str_u32(const std::string& buf, std::size_t& pos, std::string& out) {
    std::uint32_t len = 0;
    if (!read_u32_le(buf, pos, len)) return false;
    if (pos + static_cast<std::size_t>(len) > buf.size()) return false;
    out.assign(buf, pos, static_cast<std::size_t>(len));
    pos += static_cast<std::size_t>(len);
    return true;
}

std::optional<EmbeddedBundle> parse_payload_bytes(const std::string& payload) {
    if (payload.empty()) return std::nullopt;
    std::size_t pos = 0;
    unsigned char ver = static_cast<unsigned char>(payload[pos++]);
    if (ver != 1) return std::nullopt;

    EmbeddedBundle b;
    if (!read_str_u32(payload, pos, b.entry_name)) return std::nullopt;
    if (!read_str_u32(payload, pos, b.entry_source)) return std::nullopt;
    std::uint32_t file_count = 0;
    if (!read_u32_le(payload, pos, file_count)) return std::nullopt;
    if (file_count > 10000) return std::nullopt;
    for (std::uint32_t i = 0; i < file_count; ++i) {
        std::string k, v;
        if (!read_str_u32(payload, pos, k)) return std::nullopt;
        if (!read_str_u32(payload, pos, v)) return std::nullopt;
        b.files.emplace_back(std::move(k), std::move(v));
    }
    if (pos != payload.size()) return std::nullopt;
    return b;
}

std::string read_binary_file(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) throw std::runtime_error("Could not open \"" + p.string() + "\".");
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

void collect_uses_in_expr(const ExprPtr& expr, std::vector<std::string>& out);

void collect_uses_in_stmts(const std::vector<StmtPtr>& stmts, std::vector<std::string>& out) {
    for (const auto& stmt : stmts) {
        if (!stmt) continue;
        if (auto* s_use = dynamic_cast<UseStmt*>(stmt.get())) {
            if (auto* lit = dynamic_cast<LiteralExpr*>(s_use->path.get())) {
                if (lit->kind == LiteralExpr::Kind::Text && !lit->sval.empty()) {
                    out.push_back(lit->sval);
                }
            } else {
                collect_uses_in_expr(s_use->path, out);
            }
        } else if (auto* s_let = dynamic_cast<LetStmt*>(stmt.get())) {
            collect_uses_in_expr(s_let->value, out);
        } else if (auto* s_assign = dynamic_cast<AssignStmt*>(stmt.get())) {
            collect_uses_in_expr(s_assign->value, out);
        } else if (auto* s_fn = dynamic_cast<FnStmt*>(stmt.get())) {
            collect_uses_in_stmts(s_fn->body, out);
        } else if (auto* s_while = dynamic_cast<WhileStmt*>(stmt.get())) {
            collect_uses_in_expr(s_while->cond, out);
            collect_uses_in_stmts(s_while->body, out);
        } else if (auto* s_loop = dynamic_cast<LoopStmt*>(stmt.get())) {
            collect_uses_in_expr(s_loop->count, out);
            collect_uses_in_stmts(s_loop->body, out);
        } else if (auto* s_for = dynamic_cast<ForStmt*>(stmt.get())) {
            collect_uses_in_expr(s_for->iterable, out);
            collect_uses_in_stmts(s_for->body, out);
        } else if (auto* s_try = dynamic_cast<TryStmt*>(stmt.get())) {
            collect_uses_in_stmts(s_try->try_body, out);
            collect_uses_in_stmts(s_try->catch_body, out);
        } else if (auto* s_expr = dynamic_cast<ExprStmt*>(stmt.get())) {
            collect_uses_in_expr(s_expr->expr, out);
        }
    }
}

void collect_uses_in_expr(const ExprPtr& expr, std::vector<std::string>& out) {
    if (!expr) return;
    if (auto* e = dynamic_cast<IfExpr*>(expr.get())) {
        for (const auto& br : e->branches) {
            collect_uses_in_expr(br.cond, out);
            collect_uses_in_stmts(br.body, out);
        }
        collect_uses_in_stmts(e->else_body, out);
    } else if (auto* e = dynamic_cast<MatchExpr*>(expr.get())) {
        collect_uses_in_expr(e->subject, out);
        for (const auto& arm : e->arms) {
            collect_uses_in_stmts(arm.body, out);
            collect_uses_in_expr(arm.expr_value, out);
        }
    } else if (auto* e = dynamic_cast<LambdaExpr*>(expr.get())) {
        collect_uses_in_stmts(e->body, out);
    }
}

void bundle_dependencies_recursive(const Program& prog, const fs::path& base_dir,
                                 std::unordered_set<std::string>& visited,
                                 std::vector<std::pair<std::string, std::string>>& out_files) {
    std::vector<std::string> rels;
    collect_uses_in_stmts(prog.body, rels);
    for (const auto& rel : rels) {
        std::string norm = fs::path(rel).generic_string();
        if (!visited.insert(norm).second) continue;
        fs::path resolved = fs::path(rel).is_absolute() ? fs::path(rel) : (base_dir / rel);
        std::string mod_src = read_binary_file(resolved);
        Program mod_prog = parse(mod_src);
        out_files.emplace_back(norm, mod_src);
        bundle_dependencies_recursive(mod_prog, base_dir, visited, out_files);
    }
}

bool ends_with_ci(const std::string& s, const std::string& suffix) {
    if (s.size() < suffix.size()) return false;
    std::size_t off = s.size() - suffix.size();
    for (std::size_t i = 0; i < suffix.size(); ++i) {
        char a = s[off + i];
        char b = suffix[i];
        if (a >= 'A' && a <= 'Z') a = static_cast<char>(a - 'A' + 'a');
        if (b >= 'A' && b <= 'Z') b = static_cast<char>(b - 'A' + 'a');
        if (a != b) return false;
    }
    return true;
}

fs::path normalize_output_path(const fs::path& p) {
#ifdef _WIN32
    std::string s = p.string();
    if (!ends_with_ci(s, ".exe")) return fs::path(s + ".exe");
#else
    (void)ends_with_ci("", "");
#endif
    return p;
}

} // namespace

fs::path get_self_executable_path() {
#ifdef _WIN32
    std::vector<wchar_t> buf(MAX_PATH, L'\0');
    for (;;) {
        DWORD n = GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
        if (n == 0) break;
        if (n < buf.size() - 1) {
            return fs::weakly_canonical(fs::path(std::wstring(buf.data(), n)));
        }
        buf.resize(buf.size() * 2);
    }
    return {};
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    if (size == 0) return {};
    std::string buf(size, '\0');
    if (_NSGetExecutablePath(buf.data(), &size) != 0) return {};
    std::error_code ec;
    fs::path p = fs::weakly_canonical(fs::path(buf.c_str()), ec);
    return ec ? fs::path(buf.c_str()) : p;
#elif defined(__linux__)
    std::error_code ec;
    fs::path p = fs::read_symlink("/proc/self/exe", ec);
    if (!ec) return fs::weakly_canonical(p, ec);
    return {};
#else
    return {};
#endif
}

std::string encode_bundle_trailer(const EmbeddedBundle& bundle) {
    std::string payload;
    payload.push_back(static_cast<char>(1));
    write_str_u32(payload, bundle.entry_name);
    write_str_u32(payload, bundle.entry_source);
    write_u32_le(payload, static_cast<std::uint32_t>(bundle.files.size()));
    for (const auto& kv : bundle.files) {
        write_str_u32(payload, kv.first);
        write_str_u32(payload, kv.second);
    }

    std::string out;
    out.reserve(kMagicLen * 2 + 16 + payload.size());
    out.append(magic_beg());
    write_u64_le(out, static_cast<std::uint64_t>(payload.size()));
    out.append(payload);
    write_u64_le(out, static_cast<std::uint64_t>(payload.size()));
    out.append(magic_end());
    return out;
}

std::optional<EmbeddedBundle> decode_bundle_trailer(const std::string& bytes) {
    constexpr std::size_t kMinTotal = kMagicLen * 2 + 16;
    if (bytes.size() < kMinTotal) return std::nullopt;
    const std::string m_end = magic_end();
    if (bytes.compare(bytes.size() - kMagicLen, kMagicLen, m_end) != 0) {
        return std::nullopt;
    }
    std::uint64_t tail_len = decode_u64_le(&bytes[bytes.size() - kFooterTailLen]);
    std::size_t total_trailer = kMagicLen + 8 + static_cast<std::size_t>(tail_len) + kFooterTailLen;
    if (tail_len > bytes.size() || total_trailer > bytes.size()) {
        return std::nullopt;
    }
    std::size_t start = bytes.size() - total_trailer;
    const std::string m_beg = magic_beg();
    if (bytes.compare(start, kMagicLen, m_beg) != 0) {
        return std::nullopt;
    }
    std::uint64_t head_len = decode_u64_le(&bytes[start + kMagicLen]);
    if (head_len != tail_len) return std::nullopt;

    std::string payload = bytes.substr(start + kMagicLen + 8, static_cast<std::size_t>(tail_len));
    return parse_payload_bytes(payload);
}

std::size_t stripped_host_size(const std::string& bytes) {
    constexpr std::size_t kMinTotal = kMagicLen * 2 + 16;
    if (bytes.size() < kMinTotal) return bytes.size();
    if (bytes.compare(bytes.size() - kMagicLen, kMagicLen, magic_end()) != 0) {
        return bytes.size();
    }
    std::uint64_t tail_len = decode_u64_le(&bytes[bytes.size() - kFooterTailLen]);
    if (tail_len > bytes.size()) return bytes.size();
    std::size_t total_trailer = kMagicLen + 8 + static_cast<std::size_t>(tail_len) + kFooterTailLen;
    if (total_trailer > bytes.size()) return bytes.size();
    std::size_t start = bytes.size() - total_trailer;
    if (bytes.compare(start, kMagicLen, magic_beg()) != 0) return bytes.size();
    if (decode_u64_le(&bytes[start + kMagicLen]) != tail_len) return bytes.size();
    return start;
}

std::optional<EmbeddedBundle> read_embedded_bundle_from_file(const fs::path& exe_path) {
    if (exe_path.empty()) return std::nullopt;
    std::ifstream f(exe_path, std::ios::binary | std::ios::ate);
    if (!f) return std::nullopt;
    std::streamoff file_size = f.tellg();
    constexpr std::streamoff kMinTotal = static_cast<std::streamoff>(kMagicLen * 2 + 16);
    if (file_size < kMinTotal) return std::nullopt;

    f.seekg(file_size - static_cast<std::streamoff>(kFooterTailLen), std::ios::beg);
    char tail_buf[kFooterTailLen];
    if (!f.read(tail_buf, static_cast<std::streamsize>(kFooterTailLen))) return std::nullopt;
    if (std::memcmp(tail_buf + 8, magic_end().data(), kMagicLen) != 0) return std::nullopt;

    std::uint64_t tail_len = decode_u64_le(tail_buf);
    if (tail_len > static_cast<std::uint64_t>(file_size)) return std::nullopt;
    std::streamoff header_offset =
        file_size - static_cast<std::streamoff>(kMagicLen + 8) -
        static_cast<std::streamoff>(tail_len) - static_cast<std::streamoff>(kFooterTailLen);
    if (header_offset < 0) return std::nullopt;

    f.seekg(header_offset, std::ios::beg);
    std::string head_and_payload(static_cast<std::size_t>(kMagicLen + 8 + tail_len), '\0');
    if (!f.read(head_and_payload.data(), static_cast<std::streamsize>(head_and_payload.size()))) {
        return std::nullopt;
    }
    if (head_and_payload.compare(0, kMagicLen, magic_beg()) != 0) return std::nullopt;
    if (decode_u64_le(head_and_payload.data() + kMagicLen) != tail_len) return std::nullopt;

    return parse_payload_bytes(head_and_payload.substr(kMagicLen + 8));
}

std::optional<EmbeddedBundle> read_embedded_bundle() {
    fs::path self = get_self_executable_path();
    if (self.empty()) return std::nullopt;
    return read_embedded_bundle_from_file(self);
}

fs::path default_binary_output_path(const fs::path& source_path) {
    fs::path stem = source_path.stem();
    if (stem.empty()) stem = "app";
    return normalize_output_path(stem);
}

BuildBinaryResult build_standalone_binary_from_source(
    const std::string& entry_name,
    const std::string& source,
    const fs::path& base_dir,
    const fs::path& output_path,
    const fs::path& host_binary_override) {
    Program prog = parse(source);

    EmbeddedBundle bundle;
    bundle.entry_name = entry_name.empty() ? "app.kes" : entry_name;
    bundle.entry_source = source;

    fs::path effective_base = base_dir.empty() ? fs::current_path() : base_dir;
    std::unordered_set<std::string> visited;
    bundle_dependencies_recursive(prog, effective_base, visited, bundle.files);

    fs::path host = host_binary_override.empty() ? get_self_executable_path() : host_binary_override;
    if (host.empty() || !fs::exists(host)) {
        throw std::runtime_error("Could not locate the Kestral executable binary to package.");
    }

    fs::path out_p = normalize_output_path(output_path);
    std::error_code ec;
    fs::path can_host = fs::weakly_canonical(host, ec);
    fs::path can_out = fs::weakly_canonical(out_p, ec);
    if (!can_host.empty() && !can_out.empty() && can_host == can_out) {
        throw std::runtime_error("Output binary path cannot overwrite the running kestral binary.");
    }

    std::string host_bytes = read_binary_file(host);
    std::size_t base_size = stripped_host_size(host_bytes);
    std::string trailer = encode_bundle_trailer(bundle);

    if (out_p.has_parent_path()) {
        fs::create_directories(out_p.parent_path(), ec);
    }

    {
        std::ofstream wf(out_p, std::ios::binary | std::ios::trunc);
        if (!wf) throw std::runtime_error("Could not write binary to \"" + out_p.string() + "\".");
        wf.write(host_bytes.data(), static_cast<std::streamsize>(base_size));
        wf.write(trailer.data(), static_cast<std::streamsize>(trailer.size()));
        if (!wf) throw std::runtime_error("Failed writing binary to \"" + out_p.string() + "\".");
    }

#ifndef _WIN32
    fs::permissions(out_p,
                    fs::perms::owner_all | fs::perms::group_read | fs::perms::group_exec |
                        fs::perms::others_read | fs::perms::others_exec,
                    fs::perm_options::replace, ec);
#endif

    BuildBinaryResult res;
    res.output_path = out_p;
    res.size_bytes = base_size + trailer.size();
    res.bundled_file_count = bundle.files.size();
    return res;
}

BuildBinaryResult build_standalone_binary(
    const fs::path& source_path,
    const fs::path& output_path,
    const fs::path& host_binary_override) {
    std::string src = read_binary_file(source_path);
    fs::path abs_src = fs::absolute(source_path);
    fs::path out_p = output_path.empty() ? default_binary_output_path(source_path) : output_path;
    return build_standalone_binary_from_source(
        source_path.filename().string(),
        src,
        abs_src.parent_path(),
        out_p,
        host_binary_override);
}

} // namespace kestral
