#include "nana.hpp"
#include "banner.hpp"
#include "error.hpp"
#include "interp.hpp"
#include "learn.hpp"
#include "parser.hpp"
#include "version.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <unordered_set>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <conio.h>
#include <windows.h>
#else
#include <csignal>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace kestral {

namespace {

const char* RESET = "\x1b[0m";
const char* BOLD = "\x1b[1m";
const char* DIM = "\x1b[2m";
const char* BRIGHT_WHITE = "\x1b[1;97m";
const char* GRAY = "\x1b[90m";
const char* GREEN_CLR = "\x1b[92m";
const char* RED_CLR = "\x1b[91m";
const char* YELLOW_CLR = "\x1b[93m";
const char* BLUE_CLR = "\x1b[1;94m";
const char* MAGENTA_CLR = "\x1b[1;95m";
const char* CYAN_CLR = "\x1b[36m";
const char* BLACK_BG = "\x1b[40;48;2;0;0;0m";
const char* BLACK_RESET = "\x1b[0;40;48;2;0;0;0m";
const char* DEFAULT_BG = "\x1b[49m";

const char* CHEAT_SHEET =
    "Kestral Quick Reference (.kes)\n"
    "──────────────────────────────\n"
    "Output       say \"Hi {name}\", 42\n"
    "Bindings     let x = 10   |   x += 5   |   // comment\n"
    "Functions    fn add(a, b = 1) => a + b\n"
    "             fn greet(who) { give \"Hi {who}\" }\n"
    "Pipes & Ops  \"hi\" |> upper |> box   |   a ?? b   |   1..5\n"
    "Match        match n { 0 => \"zero\"  1..5 => \"few\"  else => \"many\" }\n"
    "Loops        loop 3 times { ... }   |   for x in xs { ... }\n"
    "             while cond { ... }     |   stop / skip\n"
    "Errors       try { fail \"oops\" } catch e { say e }\n"
    "CLI/TUI      args()  env(\"HOME\")  readFile(p)  writeFile(p, s)  box(s)  color(s, c)\n"
    "\n"
    "nana Shortcuts\n"
    "──────────────\n"
    "Ctrl+S  Save      Ctrl+R  Run buffer    Ctrl+P  TUI Menu\n"
    "Ctrl+K  Cut line  Ctrl+U  Paste line    Ctrl+G  Help\n"
    "Ctrl+X  Exit";

#ifndef _WIN32
volatile std::sig_atomic_t g_sigwinch = 0;
void on_sigwinch(int) { g_sigwinch = 1; }
#endif

const std::unordered_set<std::string>& hl_keywords() {
    static const std::unordered_set<std::string> s = {
        "let", "fn", "if", "else", "while", "for", "in", "loop", "times",
        "stop", "skip", "give", "try", "catch", "fail", "match", "and",
        "or", "not", "true", "false", "nothing", "say", "use"
    };
    return s;
}

const std::unordered_set<std::string>& hl_builtins() {
    static const std::unordered_set<std::string> s = {
        "ask", "type", "str", "num", "int", "list", "copy",
        "abs", "floor", "ceil", "sqrt", "round", "min", "max", "sum", "random", "randomInt",
        "len", "push", "pop", "remove", "slice", "reverse", "sort", "unique",
        "keys", "values", "entries", "contains", "find",
        "map", "filter", "reduce", "any", "all",
        "upper", "lower", "trim", "split", "join", "replace", "startsWith", "endsWith", "repeat",
        "assert", "args", "env", "fileExists", "readFile", "writeFile", "box", "color", "clear"
    };
    return s;
}

std::string pad_right_vis(const std::string& s, int width) {
    int cur = static_cast<int>(visible_columns(s));
    if (cur >= width) return s;
    return s + std::string(static_cast<std::size_t>(width - cur), ' ');
}

std::string make_hairline(int cols) {
    std::string bar;
    for (int i = 0; i < cols; ++i) bar += "─";
    return std::string(GRAY) + bar + RESET;
}

bool match_ansi_csi(const std::string& s, std::size_t i, std::size_t& end_out) {
    if (i + 1 >= s.size() || s[i] != '\x1b' || s[i + 1] != '[') return false;
    std::size_t j = i + 2;
    while (j < s.size() && (std::isdigit(static_cast<unsigned char>(s[j])) ||
                            s[j] == ';' || s[j] == '?')) {
        j++;
    }
    if (j < s.size() && ((s[j] >= 'A' && s[j] <= 'Z') || (s[j] >= 'a' && s[j] <= 'z'))) {
        end_out = j + 1;
        return true;
    }
    return false;
}

std::string replace_all(std::string s, const std::string& from, const std::string& to) {
    std::size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

} // namespace

std::string kestral_home_dir() {
    const char* ov = std::getenv("KESTRAL_HOME");
    if (ov && *ov) return std::string(ov);
#ifdef _WIN32
    const char* up = std::getenv("USERPROFILE");
    std::string base = up ? up : ".";
#else
    const char* hm = std::getenv("HOME");
    std::string base = hm ? hm : ".";
#endif
    return (fs::path(base) / ".kestral").string();
}

std::string load_bg_mode() {
    fs::path p = fs::path(kestral_home_dir()) / "nana.json";
    std::ifstream f(p, std::ios::binary);
    if (!f) return "mica";
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string txt = ss.str();
    if (txt.find("\"black\"") != std::string::npos) return "black";
    return "mica";
}

std::string save_bg_mode(const std::string& mode) {
    std::string norm = (to_lower_ascii(mode) == "black") ? "black" : "mica";
    fs::path p = fs::path(kestral_home_dir()) / "nana.json";
    std::error_code ec;
    fs::create_directories(p.parent_path(), ec);
    std::ofstream f(p, std::ios::binary);
    if (f) {
        f << "{\n  \"bg_mode\": \"" << norm << "\"\n}\n";
    }
    return norm;
}

RawTtyGuard::~RawTtyGuard() { leave(); }

void RawTtyGuard::enter() {
    if (active) return;
    enable_utf8_console();
#ifndef _WIN32
    auto* orig = new termios{};
    if (tcgetattr(STDIN_FILENO, orig) == 0) {
        termios raw = *orig;
        raw.c_iflag &= ~static_cast<tcflag_t>(IXON | ICRNL | ISTRIP | INPCK);
        raw.c_cflag |= CS8;
        raw.c_lflag &= ~static_cast<tcflag_t>(ECHO | ICANON | ISIG | IEXTEN);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 1;
        tcsetattr(STDIN_FILENO, TCSADRAIN, &raw);
        saved_state = orig;
        std::signal(SIGWINCH, on_sigwinch);
        active = true;
    } else {
        delete orig;
    }
#else
    active = true;
#endif
}

void RawTtyGuard::leave() {
    if (!active) return;
#ifndef _WIN32
    if (saved_state) {
        auto* orig = static_cast<termios*>(saved_state);
        tcsetattr(STDIN_FILENO, TCSADRAIN, orig);
        delete orig;
        saved_state = nullptr;
    }
#endif
    active = false;
}

std::pair<int, int> get_terminal_size() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi{};
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        int cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        int rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        return {std::max(rows, 10), std::max(cols, 36)};
    }
#else
    winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0 && ws.ws_col > 0) {
        return {static_cast<int>(ws.ws_row), static_cast<int>(ws.ws_col)};
    }
#endif
    return {24, 80};
}

bool stdin_stdout_are_tty() {
#ifdef _WIN32
    return GetFileType(GetStdHandle(STD_INPUT_HANDLE)) == FILE_TYPE_CHAR &&
           GetFileType(GetStdHandle(STD_OUTPUT_HANDLE)) == FILE_TYPE_CHAR;
#else
    return isatty(STDIN_FILENO) != 0 && isatty(STDOUT_FILENO) != 0;
#endif
}

std::string read_key() {
    auto last = get_terminal_size();
#ifdef _WIN32
    for (;;) {
        if (get_terminal_size() != last) return "RESIZE";
        if (_kbhit()) {
            int ch = _getwch();
            if (ch == 0 || ch == 224) {
                int code = _getwch();
                if (code == 72) return "UP";
                if (code == 80) return "DOWN";
                if (code == 75) return "LEFT";
                if (code == 77) return "RIGHT";
                if (code == 71) return "HOME";
                if (code == 79) return "END";
                if (code == 83) return "DELETE";
                return "";
            }
            if (ch >= 1 && ch < 128) return std::string(1, static_cast<char>(ch));
            return "";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
#else
    for (;;) {
        if (g_sigwinch) {
            g_sigwinch = 0;
            return "RESIZE";
        }
        if (get_terminal_size() != last) return "RESIZE";
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        timeval tv{0, 120000};
        int rv = select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv);
        if (rv < 0) {
            g_sigwinch = 0;
            return "RESIZE";
        }
        if (rv == 0) continue;
        char c = 0;
        if (read(STDIN_FILENO, &c, 1) <= 0) continue;
        if (c == '\x1b') {
            fd_set efds;
            FD_ZERO(&efds);
            FD_SET(STDIN_FILENO, &efds);
            timeval etv{0, 30000};
            if (select(STDIN_FILENO + 1, &efds, nullptr, nullptr, &etv) > 0) {
                char nxt = 0;
                if (read(STDIN_FILENO, &nxt, 1) > 0 && nxt == '[') {
                    std::string seq;
                    for (;;) {
                        fd_set sfds;
                        FD_ZERO(&sfds);
                        FD_SET(STDIN_FILENO, &sfds);
                        timeval stv{0, 30000};
                        if (select(STDIN_FILENO + 1, &sfds, nullptr, nullptr, &stv) <= 0) break;
                        char sc = 0;
                        if (read(STDIN_FILENO, &sc, 1) <= 0) break;
                        seq += sc;
                        if (std::isalpha(static_cast<unsigned char>(sc)) || sc == '~') break;
                    }
                    if (seq == "A") return "UP";
                    if (seq == "B") return "DOWN";
                    if (seq == "C") return "RIGHT";
                    if (seq == "D") return "LEFT";
                    if (seq == "H" || seq == "1~") return "HOME";
                    if (seq == "F" || seq == "4~") return "END";
                    if (seq == "3~") return "DELETE";
                    return "";
                }
            }
            return "ESC";
        }
        return std::string(1, c);
    }
#endif
}

std::string strip_ansi(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    std::size_t i = 0;
    while (i < s.size()) {
        std::size_t end = 0;
        if (s[i] == '\x1b' && match_ansi_csi(s, i, end)) {
            i = end;
            continue;
        }
        out += s[i++];
    }
    return out;
}

std::size_t visible_columns(const std::string& s) {
    // Count UTF-8 codepoints outside ANSI escapes cleanly.
    std::string plain = strip_ansi(s);
    std::size_t cnt = 0;
    for (unsigned char c : plain) {
        if ((c & 0xC0) != 0x80) cnt++;
    }
    return cnt;
}

std::string clip_ansi(const std::string& s, int max_cols) {
    if (max_cols <= 0) return "";
    std::string out;
    int vis = 0;
    bool saw_ansi = false;
    std::size_t i = 0;
    while (i < s.size()) {
        std::size_t end = 0;
        if (s[i] == '\x1b' && match_ansi_csi(s, i, end)) {
            out.append(s, i, end - i);
            saw_ansi = true;
            i = end;
            continue;
        }
        if (s[i] == '\r' || s[i] == '\n') { i++; continue; }
        unsigned char lead = static_cast<unsigned char>(s[i]);
        std::size_t clen = 1;
        if ((lead & 0xE0) == 0xC0) clen = 2;
        else if ((lead & 0xF0) == 0xE0) clen = 3;
        else if ((lead & 0xF8) == 0xF0) clen = 4;
        if (i + clen > s.size()) clen = s.size() - i;
        if (vis >= max_cols) break;
        out.append(s, i, clen);
        vis++;
        i += clen;
    }
    if (saw_ansi) out += RESET;
    return out;
}

std::string compose_screen(const std::vector<std::string>& lines,
                           int rows, int cols,
                           int cursor_row, int cursor_col,
                           const std::string& bg_mode) {
    rows = std::max(rows, 1);
    cols = std::max(cols, 1);
    bool use_black = (to_lower_ascii(bg_mode) == "black");
    std::ostringstream out;
    out << "\x1b[?2026h";
    for (int r = 0; r < rows; ++r) {
        const std::string& raw = (r < static_cast<int>(lines.size())) ? lines[static_cast<std::size_t>(r)] : "";
        std::string clipped = clip_ansi(raw, cols);
        if (use_black) {
            std::string styled = replace_all(clipped, RESET, BLACK_RESET);
            int pad = std::max(0, cols - static_cast<int>(visible_columns(clipped)));
            out << "\x1b[" << (r + 1) << ";1H" << BLACK_BG << styled
                << std::string(static_cast<std::size_t>(pad), ' ') << BLACK_BG << "\x1b[K";
        } else {
            out << "\x1b[" << (r + 1) << ";1H" << DEFAULT_BG << clipped << DEFAULT_BG << "\x1b[K";
        }
    }
    if (cursor_row >= 1 && cursor_col >= 1) {
        int cr = std::max(1, std::min(cursor_row, rows));
        int cc = std::max(1, std::min(cursor_col, cols));
        out << "\x1b[?25h\x1b[" << cr << ";" << cc << "H";
    } else {
        out << "\x1b[?25l";
    }
    out << "\x1b[?2026l";
    return out.str();
}

std::string highlight_kestral_line(const std::string& line) {
    std::string out;
    std::size_t i = 0;
    const std::size_t n = line.size();
    while (i < n) {
        char c = line[i];
        if (c == '/' && i + 1 < n && line[i + 1] == '/') {
            out += std::string(GRAY) + line.substr(i) + RESET;
            break;
        }
        if (c == '"' || c == '\'') {
            char q = c;
            std::string s = std::string(1, q);
            i++;
            out += GREEN_CLR + s;
            while (i < n) {
                char sc = line[i];
                if (sc == '\\' && i + 1 < n) {
                    out += sc;
                    out += line[i + 1];
                    i += 2;
                    continue;
                }
                if (sc == '{') {
                    std::size_t end_b = i + 1;
                    int depth = 1;
                    while (end_b < n && depth > 0) {
                        if (line[end_b] == '{') depth++;
                        else if (line[end_b] == '}') depth--;
                        end_b++;
                    }
                    out += std::string(CYAN_CLR) + line.substr(i, end_b - i) + GREEN_CLR;
                    i = end_b;
                    continue;
                }
                out += sc;
                i++;
                if (sc == q) break;
            }
            out += RESET;
            continue;
        }
        if (i + 1 < n) {
            std::string two = line.substr(i, 2);
            if (two == "=>" || two == "|>" || two == "??" || two == "..") {
                out += std::string(MAGENTA_CLR) + two + RESET;
                i += 2;
                continue;
            }
        }
        if (std::isdigit(static_cast<unsigned char>(c)) &&
            (i == 0 || !(std::isalnum(static_cast<unsigned char>(line[i - 1])) || line[i - 1] == '_'))) {
            std::size_t j = i;
            while (j < n && std::isdigit(static_cast<unsigned char>(line[j]))) j++;
            if (j + 1 < n && line[j] == '.' && std::isdigit(static_cast<unsigned char>(line[j + 1]))) {
                j++;
                while (j < n && std::isdigit(static_cast<unsigned char>(line[j]))) j++;
            }
            out += std::string(YELLOW_CLR) + line.substr(i, j - i) + RESET;
            i = j;
            continue;
        }
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            std::size_t j = i;
            while (j < n && (std::isalnum(static_cast<unsigned char>(line[j])) || line[j] == '_')) j++;
            std::string w = line.substr(i, j - i);
            if (hl_keywords().count(w)) out += std::string(BLUE_CLR) + w + RESET;
            else if (hl_builtins().count(w)) out += std::string(CYAN_CLR) + w + RESET;
            else out += w;
            i = j;
            continue;
        }
        out += c;
        i++;
    }
    return out;
}

std::vector<std::string> discover_kes_files(const std::string& root_dir, std::size_t limit) {
    std::vector<std::string> out;
    fs::path base = fs::weakly_canonical(root_dir);
    std::error_code ec;
    if (!fs::is_directory(base, ec)) return out;
    static const std::unordered_set<std::string> skip = {
        ".git", "build", "node_modules", ".venv", "dist"
    };
    fs::recursive_directory_iterator it(base, fs::directory_options::skip_permission_denied, ec);
    for (; it != fs::recursive_directory_iterator(); ++it) {
        if (it.depth() > 3) { it.disable_recursion_pending(); continue; }
        std::string fname = it->path().filename().string();
        if (it->is_directory(ec)) {
            if (skip.count(fname) || (!fname.empty() && fname[0] == '.')) {
                it.disable_recursion_pending();
            }
            continue;
        }
        if (it->path().extension() == ".kes") {
            out.push_back(fs::relative(it->path(), base, ec).string());
            if (out.size() >= limit) break;
        }
    }
    std::sort(out.begin(), out.end());
    return out;
}

const std::vector<NanaTemplate>& nana_templates() {
    static const std::vector<NanaTemplate> tpls = {
        {
            "hello",
            "Hello Kestral",
            "Interpolation, functions, and dot-method calls",
            "hello.kes",
            "// Welcome to Kestral!\n"
            "fn greet(name = \"friend\") => \"Hello, {name}!\"\n\n"
            "for person in [\"Ada\", \"Grace\", \"Linus\"] {\n"
            "  say greet(person)\n"
            "}\n"
        },
        {
            "pipes",
            "Pipes & Methods",
            "Transform lists & text cleanly with |> and .methods",
            "pipes.kes",
            "// Pipelines and method chaining\n"
            "fn double(n) => n * 2\n"
            "let nums = [5, 1, 4, 2, 3]\n"
            "let result = nums.sort().reverse() |> map(double)\n"
            "say result |> box(\"Doubled & Sorted\")\n"
        },
        {
            "match",
            "Match & Ranges",
            "Pattern matching values and inclusive integer ranges",
            "score.kes",
            "fn grade(score) {\n"
            "  give match score {\n"
            "    90..100 => \"A\"\n"
            "    80..89 => \"B\"\n"
            "    70..79 => \"C\"\n"
            "    else => \"Keep practicing\"\n"
            "  }\n"
            "}\n"
            "for s in [95, 84, 62] {\n"
            "  say \"{s} -> {grade(s)}\"\n"
            "}\n"
        },
        {
            "closures",
            "Closures & Maps",
            "Stateful closures stored inside a map record",
            "counter.kes",
            "fn makeCounter(start = 0) {\n"
            "  let n = start\n"
            "  give {\n"
            "    inc: fn(step = 1) { n += step; give n },\n"
            "    value: fn() => n\n"
            "  }\n"
            "}\n"
            "let c = makeCounter(10)\n"
            "say c.inc(), c.inc(5), c.value()\n"
        },
        {
            "cli_card",
            "CLI Status Card",
            "Use box(), color(), and args() in terminal scripts",
            "card.kes",
            "let cliArgs = args()\n"
            "let target = if len(cliArgs) > 0 { cliArgs[0] } else { \"local\" }\n"
            "let card = \"Target: {target}\\nStatus: ready\" |> box(\"Kestral CLI\")\n"
            "say color(card, \"cyan\")\n"
        },
        {
            "primes",
            "Prime Finder",
            "Fast filter over ranges using Kestral functions",
            "primes.kes",
            "fn isPrime(n) {\n"
            "  if n < 2 { give false }\n"
            "  for d in 2..n {\n"
            "    if d * d > n { stop }\n"
            "    if n % d == 0 { give false }\n"
            "  }\n"
            "  give true\n"
            "}\n"
            "let ps = filter(1..40, isPrime)\n"
            "say join(ps, \" \")\n"
        }
    };
    return tpls;
}

NanaBuffer NanaBuffer::from_file(const std::string& path) {
    NanaBuffer b;
    b.filepath = path;
    std::ifstream f(path, std::ios::binary);
    if (f) {
        std::ostringstream ss;
        ss << f.rdbuf();
        std::string raw = replace_all(ss.str(), "\r\n", "\n");
        b.lines.clear();
        std::istringstream iss(raw);
        std::string ln;
        while (std::getline(iss, ln)) b.lines.push_back(ln);
        if (b.lines.empty()) b.lines.push_back("");
    } else if (!path.empty()) {
        b.status = "New file: " + path;
    }
    return b;
}

NanaBuffer NanaBuffer::from_template(const NanaTemplate& tpl) {
    NanaBuffer b;
    b.filepath = tpl.filename;
    b.lines.clear();
    std::istringstream iss(tpl.code);
    std::string ln;
    while (std::getline(iss, ln)) b.lines.push_back(ln);
    if (b.lines.empty()) b.lines.push_back("");
    b.modified = true;
    b.status = "Template: " + tpl.title;
    return b;
}

std::string NanaBuffer::text() const {
    std::string out;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (i > 0) out += '\n';
        out += lines[i];
    }
    return out;
}

void NanaBuffer::clamp_col() {
    int mx = static_cast<int>(lines[static_cast<std::size_t>(row)].size());
    col = std::max(0, std::min(col, mx));
}

void NanaBuffer::insert_char(char ch) {
    auto& ln = lines[static_cast<std::size_t>(row)];
    ln.insert(static_cast<std::size_t>(col), 1, ch);
    col++;
    modified = true;
}

void NanaBuffer::insert_tab() {
    auto& ln = lines[static_cast<std::size_t>(row)];
    ln.insert(static_cast<std::size_t>(col), "  ");
    col += 2;
    modified = true;
}

void NanaBuffer::insert_newline() {
    std::string cur = lines[static_cast<std::size_t>(row)];
    std::string before = cur.substr(0, static_cast<std::size_t>(col));
    std::string after = cur.substr(static_cast<std::size_t>(col));
    std::size_t ind = 0;
    while (ind < before.size() && before[ind] == ' ') ind++;
    std::string indent(ind, ' ');
    std::size_t end = before.size();
    while (end > 0 && before[end - 1] == ' ') end--;
    if (end > 0 && before[end - 1] == '{') indent += "  ";
    lines[static_cast<std::size_t>(row)] = before;
    lines.insert(lines.begin() + row + 1, indent + after);
    row++;
    col = static_cast<int>(indent.size());
    modified = true;
}

void NanaBuffer::backspace() {
    if (col > 0) {
        auto& ln = lines[static_cast<std::size_t>(row)];
        ln.erase(static_cast<std::size_t>(col - 1), 1);
        col--;
        modified = true;
    } else if (row > 0) {
        std::string cur = lines[static_cast<std::size_t>(row)];
        lines.erase(lines.begin() + row);
        row--;
        col = static_cast<int>(lines[static_cast<std::size_t>(row)].size());
        lines[static_cast<std::size_t>(row)] += cur;
        modified = true;
    }
}

void NanaBuffer::del() {
    auto& ln = lines[static_cast<std::size_t>(row)];
    if (col < static_cast<int>(ln.size())) {
        ln.erase(static_cast<std::size_t>(col), 1);
        modified = true;
    } else if (row + 1 < static_cast<int>(lines.size())) {
        ln += lines[static_cast<std::size_t>(row + 1)];
        lines.erase(lines.begin() + row + 1);
        modified = true;
    }
}

void NanaBuffer::move_left() {
    if (col > 0) col--;
    else if (row > 0) {
        row--;
        col = static_cast<int>(lines[static_cast<std::size_t>(row)].size());
    }
}

void NanaBuffer::move_right() {
    if (col < static_cast<int>(lines[static_cast<std::size_t>(row)].size())) col++;
    else if (row + 1 < static_cast<int>(lines.size())) {
        row++;
        col = 0;
    }
}

void NanaBuffer::move_up() { if (row > 0) { row--; clamp_col(); } }
void NanaBuffer::move_down() { if (row + 1 < static_cast<int>(lines.size())) { row++; clamp_col(); } }
void NanaBuffer::move_home() { col = 0; }
void NanaBuffer::move_end() { col = static_cast<int>(lines[static_cast<std::size_t>(row)].size()); }

void NanaBuffer::cut_line() {
    cut_buffer = lines[static_cast<std::size_t>(row)];
    has_cut = true;
    if (lines.size() == 1) { lines[0].clear(); col = 0; }
    else {
        lines.erase(lines.begin() + row);
        if (row >= static_cast<int>(lines.size())) row = static_cast<int>(lines.size()) - 1;
        clamp_col();
    }
    modified = true;
    status = "Cut line";
}

void NanaBuffer::paste_line() {
    if (!has_cut) return;
    lines.insert(lines.begin() + row, cut_buffer);
    row++;
    clamp_col();
    modified = true;
    status = "Pasted line";
}

std::pair<bool, std::string> NanaBuffer::check_syntax() const {
    std::string src = text();
    bool empty = true;
    for (char c : src) {
        if (!std::isspace(static_cast<unsigned char>(c))) { empty = false; break; }
    }
    if (empty) return {true, "Ready"};
    try {
        parse(src);
        return {true, "Syntax OK"};
    } catch (const KestralError& err) {
        std::string loc = (err.line > 0) ? ("Line " + std::to_string(err.line) + ": ") : "";
        return {false, loc + err.what()};
    }
}

std::string NanaBuffer::save(const std::string& target) {
    std::string dest = target.empty() ? filepath : target;
    if (dest.empty()) throw std::runtime_error("No filename specified.");
    fs::path p(dest);
    if (!p.parent_path().empty()) {
        std::error_code ec;
        fs::create_directories(p.parent_path(), ec);
    }
    std::string content = text();
    if (!content.empty() && content.back() != '\n') content += '\n';
    std::ofstream f(dest, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot write \"" + dest + "\".");
    f << content;
    filepath = dest;
    modified = false;
    status = "Saved " + dest;
    return dest;
}

std::string NanaBuffer::render_frame(int rows, int cols, const std::string& bg_mode) {
    rows = std::max(rows, 8);
    cols = std::max(cols, 36);
    int body_rows = rows - 4;
    int gutter_w = std::max(static_cast<int>(std::to_string(lines.size()).size()) + 3, 6);
    int code_w = std::max(cols - gutter_w, 10);

    if (row < scroll_y) scroll_y = row;
    else if (row >= scroll_y + body_rows) scroll_y = row - body_rows + 1;
    if (col < scroll_x) scroll_x = col;
    else if (col >= scroll_x + code_w) scroll_x = col - code_w + 1;

    std::string name = filepath.empty() ? "[untitled.kes]" : filepath;
    std::string mod_tag = modified ? " *" : "";
    std::string right_hdr = "Ln " + std::to_string(row + 1) + ", Col " + std::to_string(col + 1) +
                            " · v" + KESTRAL_VERSION + " ";
    int max_left = std::max(cols - static_cast<int>(right_hdr.size()) - 4, 10);
    std::string left_plain = (" NANA · " + name + mod_tag).substr(0, static_cast<std::size_t>(max_left));
    int gap = std::max(cols - static_cast<int>(left_plain.size()) - static_cast<int>(right_hdr.size()), 1);
    std::string header = std::string(" ") + BOLD + "NANA" + RESET + " " + GRAY + "· " +
                         name.substr(0, static_cast<std::size_t>(std::max(max_left - 8, 6))) + mod_tag +
                         RESET + std::string(static_cast<std::size_t>(gap), ' ') + GRAY + right_hdr + RESET;

    std::vector<std::string> screen{header, make_hairline(cols)};
    int num_w = gutter_w - 3;
    for (int vr = 0; vr < body_rows; ++vr) {
        int idx = scroll_y + vr;
        if (idx < static_cast<int>(lines.size())) {
            const auto& raw = lines[static_cast<std::size_t>(idx)];
            std::string sliced = (scroll_x < static_cast<int>(raw.size()))
                                     ? raw.substr(static_cast<std::size_t>(scroll_x),
                                                  static_cast<std::size_t>(code_w))
                                     : "";
            std::string lno = std::to_string(idx + 1);
            if (static_cast<int>(lno.size()) < num_w) {
                lno = std::string(static_cast<std::size_t>(num_w - static_cast<int>(lno.size())), ' ') + lno;
            }
            screen.push_back(std::string(GRAY) + lno + " │ " + RESET + highlight_kestral_line(sliced));
        } else {
            std::string tilde = "~";
            if (num_w > 1) tilde = std::string(static_cast<std::size_t>(num_w - 1), ' ') + "~";
            screen.push_back(std::string(GRAY) + tilde + " │ " + RESET);
        }
    }

    auto syn = check_syntax();
    std::string badge = syn.first ? (std::string(GREEN_CLR) + "✓ " + syn.second + RESET)
                                  : (std::string(RED_CLR) + "✗ " + syn.second + RESET);
    std::string extra = status.empty() ? "" : ("  │  " + status);
    screen.push_back(" " + badge + extra);

    std::string footer = " ^S Save   ^R Run   ^P Menu   ^K Cut   ^U Paste   ^G Help   ^X Exit ";
    screen.push_back(std::string(DIM) + footer.substr(0, static_cast<std::size_t>(cols)) + RESET);

    int cr = (row - scroll_y) + 3;
    int cc = gutter_w + (col - scroll_x) + 1;
    return compose_screen(screen, rows, cols, cr, cc, bg_mode);
}

static std::vector<std::tuple<std::string, std::string, std::string>> welcome_items(
    bool has_active_buffer, const std::string& bg_mode) {
    std::vector<std::tuple<std::string, std::string, std::string>> items;
    if (has_active_buffer) {
        items.emplace_back("resume", "Resume Editor", "Return to your open Kestral buffer");
    }
    items.emplace_back("new", "New File", "Start a blank .kes Kestral script");
    items.emplace_back("template", "Start from Template", "Pipes, match, closures & CLI starters");
    items.emplace_back("open", "Open File", "Browse .kes scripts in the current folder");
    items.emplace_back("learn", "Learn Kestral", "Interactive CLI-first Kestral TUI course");
    std::string bg_lbl = (bg_mode == "mica") ? "Surface: Mica (Transparent)" : "Surface: Black (#000000)";
    items.emplace_back("bg", bg_lbl, "Switch transparent Mica vs pure Black [t]");
    items.emplace_back("cheat", "Kestral Cheat Sheet", "Syntax, pipes, match & built-ins reference");
    items.emplace_back("quit", "Quit", "Exit nana");
    return items;
}

std::string render_welcome_menu(int selected, int rows, int cols,
                                bool has_active_buffer, const std::string& bg_mode) {
    rows = std::max(rows, 10);
    cols = std::max(cols, 36);
    auto items = welcome_items(has_active_buffer, bg_mode);
    int sel = std::max(0, std::min(selected, static_cast<int>(items.size()) - 1));

    std::string bg_tag = (bg_mode == "black") ? "black" : "mica";
    std::string right_top = std::string("Kestral v") + KESTRAL_VERSION + " · " + bg_tag + " ";
    int gap_top = std::max(cols - 16 - static_cast<int>(right_top.size()), 1);
    std::string header_bar = std::string(" ") + BOLD + "NANA" + RESET + " " + GRAY + "· studio" + RESET +
                             std::string(static_cast<std::size_t>(gap_top), ' ') + GRAY + right_top + RESET;
    std::string hairline = make_hairline(cols);
    std::string footer_bar = std::string(" ") + DIM + "↑/↓ Move   Enter Select   1-9 Quick Pick   t Surface   Esc/q Quit" + RESET;

    int panel_w = (cols >= 56) ? std::min(cols - 4, 74) : (cols - 2);
    std::string margin(static_cast<std::size_t>(std::max((cols - panel_w) / 2, 1)), ' ');

    std::vector<std::string> content;
    if (rows >= 18) {
        if (rows >= 22) content.push_back("");
        content.push_back(margin + BOLD + "╭─────────────╮" + RESET + "   " + BOLD + "NANA" + RESET + "  " + GRAY + "Kestral v" + KESTRAL_VERSION + RESET);
        content.push_back(margin + BOLD + "│ ╷ ╷ ╭─╴ ╭─╮ │" + RESET + "   " + GRAY + "native C++20 editor · .kes" + RESET);
        content.push_back(margin + BOLD + "│ ├─╯ ├─╴ ╰─╮ │" + RESET + "   " + GRAY + "adaptive tui · " + bg_tag + " surface" + RESET);
        content.push_back(margin + BOLD + "│ ╵ ╵ ╰─╴ ╰─╯ │" + RESET);
        content.push_back(margin + BOLD + "╰─────────────╯" + RESET);
        content.push_back("");
    } else {
        content.push_back(margin + BOLD + "NANA · Kestral v" + KESTRAL_VERSION + RESET);
        content.push_back("");
    }

    int avail = std::max(rows - 4 - static_cast<int>(content.size()), 3);
    int scroll = (sel >= avail) ? (sel - avail + 1) : 0;
    int title_w = std::min(28, std::max(panel_w - 8, 12));
    bool show_desc = (panel_w >= 54);

    for (int idx = scroll; idx < std::min(static_cast<int>(items.size()), scroll + avail); ++idx) {
        const auto& title = std::get<1>(items[static_cast<std::size_t>(idx)]);
        const auto& desc = std::get<2>(items[static_cast<std::size_t>(idx)]);
        std::string num = std::to_string(idx + 1);
        std::string t_txt = pad_right_vis(title.substr(0, static_cast<std::size_t>(title_w)), title_w);
        std::string d_txt = show_desc ? desc.substr(0, static_cast<std::size_t>(std::max(panel_w - title_w - 8, 8))) : "";
        if (idx == sel) {
            content.push_back(margin + BRIGHT_WHITE + "▌ " + num + ". " + t_txt + RESET +
                              (d_txt.empty() ? "" : (" \x1b[37m" + d_txt + RESET)));
        } else {
            content.push_back(margin + "  " + GRAY + num + "." + RESET + " \x1b[37m" + t_txt + RESET +
                              (d_txt.empty() ? "" : (" " + std::string(GRAY) + d_txt + RESET)));
        }
    }

    std::vector<std::string> lines{header_bar, hairline};
    lines.insert(lines.end(), content.begin(), content.end());
    while (static_cast<int>(lines.size()) < rows - 2) lines.push_back("");
    if (static_cast<int>(lines.size()) > rows - 2) lines.resize(static_cast<std::size_t>(rows - 2));
    lines.push_back(hairline);
    lines.push_back(footer_bar);
    return compose_screen(lines, rows, cols, -1, -1, bg_mode);
}

std::string render_template_picker(int selected, int rows, int cols, const std::string& bg_mode) {
    rows = std::max(rows, 10);
    cols = std::max(cols, 36);
    const auto& tpls = nana_templates();
    int sel = std::max(0, std::min(selected, static_cast<int>(tpls.size()) - 1));
    const auto& tpl = tpls[static_cast<std::size_t>(sel)];

    std::string header_bar = std::string(" ") + BOLD + "NANA" + RESET + " " + GRAY + "· Start from Template" + RESET;
    std::string hairline = make_hairline(cols);
    std::string footer_bar = std::string(" ") + DIM + "↑/↓ Choose Template   Enter Open in Editor   t Surface   Esc Back" + RESET;
    int body_rows = rows - 4;

    std::vector<std::string> preview;
    std::istringstream iss(tpl.code);
    std::string pl;
    while (std::getline(iss, pl)) preview.push_back(pl);

    if (cols >= 72) {
        int left_w = std::max(26, std::min(40, cols * 36 / 100));
        int right_w = std::max(cols - left_w - 2, 20);
        int cap = std::max(body_rows - 5, 3);
        int scroll = std::max(0, sel - cap + 1);

        std::vector<std::string> lcol{"", std::string(" ") + BOLD + "Templates" + RESET, ""};
        for (int idx = scroll; idx < std::min(static_cast<int>(tpls.size()), scroll + cap); ++idx) {
            std::string lbl = (std::to_string(idx + 1) + ". " + tpls[static_cast<std::size_t>(idx)].title)
                                  .substr(0, static_cast<std::size_t>(std::max(left_w - 5, 10)));
            if (idx == sel) lcol.push_back(std::string(" ") + BRIGHT_WHITE + "▌ " + lbl + RESET);
            else lcol.push_back(std::string("   \x1b[37m") + lbl + RESET);
        }
        lcol.push_back("");
        lcol.push_back(std::string(" ") + GRAY + tpl.description.substr(0, static_cast<std::size_t>(std::max(left_w - 2, 10))) + RESET);

        std::vector<std::string> rcol{
            "",
            std::string(" ") + BOLD + "Preview: " + tpl.filename + RESET,
            std::string(" ") + make_hairline(std::min(right_w - 2, 46)),
        };
        for (const auto& ln : preview) {
            rcol.push_back(" " + highlight_kestral_line(ln.substr(0, static_cast<std::size_t>(std::max(right_w - 2, 8)))));
        }

        std::vector<std::string> screen{header_bar, hairline};
        for (int r = 0; r < body_rows; ++r) {
            std::string l = (r < static_cast<int>(lcol.size())) ? clip_ansi(lcol[static_cast<std::size_t>(r)], left_w) : "";
            std::string rv = (r < static_cast<int>(rcol.size())) ? clip_ansi(rcol[static_cast<std::size_t>(r)], right_w) : "";
            screen.push_back(pad_right_vis(l, left_w) + GRAY + "│" + RESET + rv);
        }
        screen.push_back(hairline);
        screen.push_back(footer_bar);
        return compose_screen(screen, rows, cols, -1, -1, bg_mode);
    }

    std::vector<std::string> screen{header_bar, hairline};
    int list_cap = std::max(std::min(static_cast<int>(tpls.size()), body_rows / 2), 2);
    int scroll = std::max(0, sel - list_cap + 1);
    for (int idx = scroll; idx < std::min(static_cast<int>(tpls.size()), scroll + list_cap); ++idx) {
        std::string lbl = (std::to_string(idx + 1) + ". " + tpls[static_cast<std::size_t>(idx)].title)
                              .substr(0, static_cast<std::size_t>(std::max(cols - 6, 10)));
        if (idx == sel) screen.push_back(std::string(" ") + BRIGHT_WHITE + "▌ " + lbl + RESET);
        else screen.push_back(std::string("   \x1b[37m") + lbl + RESET);
    }
    if (static_cast<int>(screen.size()) + 3 < rows - 2) {
        screen.push_back("");
        screen.push_back(std::string(" ") + BOLD + "Preview (" + tpl.filename + "):" + RESET);
        for (const auto& ln : preview) {
            if (static_cast<int>(screen.size()) >= rows - 2) break;
            screen.push_back(" " + highlight_kestral_line(ln.substr(0, static_cast<std::size_t>(std::max(cols - 2, 10)))));
        }
    }
    while (static_cast<int>(screen.size()) < rows - 2) screen.push_back("");
    screen.resize(static_cast<std::size_t>(rows - 2));
    screen.push_back(hairline);
    screen.push_back(footer_bar);
    return compose_screen(screen, rows, cols, -1, -1, bg_mode);
}

std::string render_file_picker(const std::vector<std::string>& files, int selected,
                               int rows, int cols, const std::string& root_dir,
                               const std::string& bg_mode) {
    rows = std::max(rows, 10);
    cols = std::max(cols, 36);
    int total = static_cast<int>(files.size()) + 1;
    int sel = std::max(0, std::min(selected, total - 1));

    std::string rname = fs::weakly_canonical(root_dir).filename().string();
    std::string header_bar = std::string(" ") + BOLD + "NANA" + RESET + " " + GRAY + "· Open .kes File (" + rname + ")" + RESET;
    std::string hairline = make_hairline(cols);
    std::string footer_bar = std::string(" ") + DIM + "↑/↓ Select   Enter Open   / Type Path   t Surface   Esc Back" + RESET;
    int body_rows = rows - 4;

    std::vector<std::string> labels = files;
    labels.push_back("[Type custom path...]");

    std::string prev_title = "Custom Path";
    std::vector<std::string> prev_lines{"Press Enter or '/' to type any file path."};
    if (sel < static_cast<int>(files.size())) {
        prev_title = labels[static_cast<std::size_t>(sel)];
        fs::path fp = fs::path(root_dir) / prev_title;
        std::ifstream f(fp, std::ios::binary);
        if (f) {
            prev_lines.clear();
            std::string ln;
            while (std::getline(f, ln) && static_cast<int>(prev_lines.size()) < std::max(body_rows, 25)) {
                if (!ln.empty() && ln.back() == '\r') ln.pop_back();
                prev_lines.push_back(ln);
            }
            if (prev_lines.empty()) prev_lines.push_back("");
        } else {
            prev_lines = {"(Cannot read file)"};
        }
    }

    int cap = std::max(body_rows - 4, 3);
    int scroll = std::max(0, sel - cap + 1);

    if (cols >= 72) {
        int left_w = std::max(26, std::min(40, cols * 36 / 100));
        int right_w = std::max(cols - left_w - 2, 20);
        std::vector<std::string> lcol{"", std::string(" ") + BOLD + ".kes Files" + RESET, ""};
        for (int idx = scroll; idx < std::min(static_cast<int>(labels.size()), scroll + cap); ++idx) {
            std::string lbl = labels[static_cast<std::size_t>(idx)].substr(0, static_cast<std::size_t>(std::max(left_w - 5, 8)));
            if (idx == sel) lcol.push_back(std::string(" ") + BRIGHT_WHITE + "▌ " + lbl + RESET);
            else lcol.push_back(std::string("   \x1b[37m") + lbl + RESET);
        }
        std::vector<std::string> rcol{
            "",
            std::string(" ") + BOLD + "Preview: " + prev_title + RESET,
            std::string(" ") + make_hairline(std::min(right_w - 2, 46)),
        };
        for (const auto& ln : prev_lines) {
            rcol.push_back(" " + highlight_kestral_line(ln.substr(0, static_cast<std::size_t>(std::max(right_w - 2, 8)))));
        }
        std::vector<std::string> screen{header_bar, hairline};
        for (int r = 0; r < body_rows; ++r) {
            std::string l = (r < static_cast<int>(lcol.size())) ? clip_ansi(lcol[static_cast<std::size_t>(r)], left_w) : "";
            std::string rv = (r < static_cast<int>(rcol.size())) ? clip_ansi(rcol[static_cast<std::size_t>(r)], right_w) : "";
            screen.push_back(pad_right_vis(l, left_w) + GRAY + "│" + RESET + rv);
        }
        screen.push_back(hairline);
        screen.push_back(footer_bar);
        return compose_screen(screen, rows, cols, -1, -1, bg_mode);
    }

    std::vector<std::string> screen{header_bar, hairline};
    for (int idx = scroll; idx < std::min(static_cast<int>(labels.size()), scroll + cap); ++idx) {
        std::string lbl = labels[static_cast<std::size_t>(idx)].substr(0, static_cast<std::size_t>(std::max(cols - 6, 10)));
        if (idx == sel) screen.push_back(std::string(" ") + BRIGHT_WHITE + "▌ " + lbl + RESET);
        else screen.push_back(std::string("   \x1b[37m") + lbl + RESET);
    }
    while (static_cast<int>(screen.size()) < rows - 2) screen.push_back("");
    screen.resize(static_cast<std::size_t>(rows - 2));
    screen.push_back(hairline);
    screen.push_back(footer_bar);
    return compose_screen(screen, rows, cols, -1, -1, bg_mode);
}

std::string prompt_bar(const std::string& label, const std::string& def,
                       NanaBuffer* buf, const std::string& bg_mode, bool* canceled) {
    if (canceled) *canceled = false;
    std::string val = def;
    for (;;) {
        auto [rows, cols] = get_terminal_size();
        if (buf) {
            buf->status.clear();
            std::cout << buf->render_frame(rows, cols, bg_mode);
        }
        int prow = buf ? (rows - 1) : rows;
        std::string raw = " ▸ " + label + val;
        std::string clipped = clip_ansi(raw, cols);
        int pcol = std::min(static_cast<int>(visible_columns(clipped)) + 1, cols);
        std::string bg = (bg_mode == "black") ? BLACK_BG : DEFAULT_BG;
        std::cout << "\x1b[" << prow << ";1H" << bg << BRIGHT_WHITE
                  << pad_right_vis(clipped, cols) << RESET << bg << "\x1b[K"
                  << "\x1b[?25h\x1b[" << prow << ";" << pcol << "H" << std::flush;
        std::string k = read_key();
        if (k == "RESIZE") continue;
        if (k == "\r" || k == "\n") return val;
        if (k == "ESC" || k == "\x03") {
            if (canceled) *canceled = true;
            return "";
        }
        if ((k == "\x7f" || k == "\x08") && !val.empty()) val.pop_back();
        else if (k.size() == 1 && static_cast<unsigned char>(k[0]) >= 32 && k[0] != 127) {
            val += k[0];
        }
    }
}

void show_overlay(const std::string& title, const std::string& body, const std::string& bg_mode) {
    std::vector<std::string> lines;
    std::istringstream iss(body);
    std::string ln;
    while (std::getline(iss, ln)) lines.push_back(ln);
    if (lines.empty()) lines.push_back("(no output)");
    int scroll = 0;
    for (;;) {
        auto [rows, cols] = get_terminal_size();
        int cap = std::max(rows - 5, 1);
        int max_scroll = std::max(static_cast<int>(lines.size()) - cap, 0);
        scroll = std::max(0, std::min(scroll, max_scroll));
        std::string hairline = make_hairline(cols);
        std::vector<std::string> screen{std::string(" ") + BOLD + title + RESET, hairline};
        for (int i = 0; i < cap && scroll + i < static_cast<int>(lines.size()); ++i) {
            screen.push_back(" " + lines[static_cast<std::size_t>(scroll + i)]);
        }
        while (static_cast<int>(screen.size()) < rows - 2) screen.push_back("");
        std::string hint = (max_scroll > 0) ? " ↑/↓ Scroll   Any other key to return... "
                                            : " Press any key to return... ";
        screen.push_back(hairline);
        screen.push_back(std::string(DIM) + hint + RESET);
        std::cout << compose_screen(screen, rows, cols, -1, -1, bg_mode) << std::flush;
        std::string k = read_key();
        if (k == "RESIZE") continue;
        if (k == "UP" && max_scroll > 0) { scroll = std::max(0, scroll - 1); continue; }
        if (k == "DOWN" && max_scroll > 0) { scroll = std::min(max_scroll, scroll + 1); continue; }
        return;
    }
}

static void run_buffer_in_nana(const NanaBuffer& buf, const std::string& bg_mode) {
    InterpConfig cfg;
    fs::path base = buf.filepath.empty() ? fs::current_path() : fs::absolute(buf.filepath).parent_path();
    cfg.read_file_fn = [base](const std::string& rel) {
        std::ifstream f(base / rel, std::ios::binary);
        if (!f) throw std::runtime_error("file not found");
        std::ostringstream ss;
        ss << f.rdbuf();
        return ss.str();
    };
    auto res = run_captured(buf.text(), {}, cfg);
    std::string out = res.ok ? (res.text.empty() ? "(finished with no output)" : res.text)
                             : res.error_text;
    show_overlay("Run Output (Kestral)", out, bg_mode);
}

static bool select_template_tui(std::string& bg_mode, NanaBuffer& out_buf) {
    int sel = 0;
    const auto& tpls = nana_templates();
    for (;;) {
        auto [rows, cols] = get_terminal_size();
        std::cout << render_template_picker(sel, rows, cols, bg_mode) << std::flush;
        std::string k = read_key();
        if (k == "RESIZE") continue;
        if (k == "t") { bg_mode = save_bg_mode(bg_mode == "mica" ? "black" : "mica"); continue; }
        if (k == "UP") sel = (sel - 1 + static_cast<int>(tpls.size())) % static_cast<int>(tpls.size());
        else if (k == "DOWN") sel = (sel + 1) % static_cast<int>(tpls.size());
        else if (k == "\r" || k == "\n") { out_buf = NanaBuffer::from_template(tpls[static_cast<std::size_t>(sel)]); return true; }
        else if (k.size() == 1 && k[0] >= '1' && k[0] <= '9') {
            int idx = k[0] - '1';
            if (idx < static_cast<int>(tpls.size())) { out_buf = NanaBuffer::from_template(tpls[static_cast<std::size_t>(idx)]); return true; }
        } else if (k == "ESC" || k == "q" || k == "\x03") return false;
    }
}

static bool select_file_tui(std::string& bg_mode, NanaBuffer& out_buf) {
    std::string root = fs::current_path().string();
    auto files = discover_kes_files(root);
    int sel = 0;
    int total = static_cast<int>(files.size()) + 1;
    for (;;) {
        auto [rows, cols] = get_terminal_size();
        std::cout << render_file_picker(files, sel, rows, cols, root, bg_mode) << std::flush;
        std::string k = read_key();
        if (k == "RESIZE") continue;
        if (k == "t") { bg_mode = save_bg_mode(bg_mode == "mica" ? "black" : "mica"); continue; }
        if (k == "UP") sel = (sel - 1 + total) % total;
        else if (k == "DOWN") sel = (sel + 1) % total;
        else if (k == "/") {
            bool canc = false;
            std::string p = prompt_bar("Open file path: ", "", nullptr, bg_mode, &canc);
            if (!canc && !p.empty()) { out_buf = NanaBuffer::from_file(p); return true; }
        } else if (k == "\r" || k == "\n") {
            if (sel < static_cast<int>(files.size())) {
                out_buf = NanaBuffer::from_file(files[static_cast<std::size_t>(sel)]);
                return true;
            }
            bool canc = false;
            std::string p = prompt_bar("Open file path: ", "", nullptr, bg_mode, &canc);
            if (!canc && !p.empty()) { out_buf = NanaBuffer::from_file(p); return true; }
        } else if (k == "ESC" || k == "q" || k == "\x03") return false;
    }
}

static bool run_menu_tui(NanaBuffer* active, bool& has_active, std::string& bg_mode, NanaBuffer& out_buf) {
    int sel = 0;
    for (;;) {
        auto items = welcome_items(has_active, bg_mode);
        sel = std::max(0, std::min(sel, static_cast<int>(items.size()) - 1));
        auto [rows, cols] = get_terminal_size();
        std::cout << render_welcome_menu(sel, rows, cols, has_active, bg_mode) << std::flush;
        std::string k = read_key();
        if (k == "RESIZE") continue;
        if (k == "t") { bg_mode = save_bg_mode(bg_mode == "mica" ? "black" : "mica"); continue; }

        std::string action;
        if (k == "UP") sel = (sel - 1 + static_cast<int>(items.size())) % static_cast<int>(items.size());
        else if (k == "DOWN") sel = (sel + 1) % static_cast<int>(items.size());
        else if (k == "\r" || k == "\n") action = std::get<0>(items[static_cast<std::size_t>(sel)]);
        else if (k.size() == 1 && k[0] >= '1' && k[0] <= '9') {
            int idx = k[0] - '1';
            if (idx < static_cast<int>(items.size())) action = std::get<0>(items[static_cast<std::size_t>(idx)]);
        } else if (k == "ESC" || k == "q" || k == "\x18" || k == "\x03") {
            if (has_active && active) { out_buf = *active; return true; }
            return false;
        }

        if (action == "resume" && has_active && active) { out_buf = *active; return true; }
        if (action == "new") {
            bool canc = false;
            std::string fn = prompt_bar("New file name (Enter for untitled): ", "", nullptr, bg_mode, &canc);
            if (canc) continue;
            out_buf = NanaBuffer();
            out_buf.filepath = fn;
            return true;
        }
        if (action == "template") {
            if (select_template_tui(bg_mode, out_buf)) return true;
        }
        if (action == "open") {
            if (select_file_tui(bg_mode, out_buf)) return true;
        }
        if (action == "learn") {
            run_learn(0, false, false, bg_mode, true);
            bg_mode = load_bg_mode();
        }
        if (action == "bg") {
            bg_mode = save_bg_mode(bg_mode == "mica" ? "black" : "mica");
        }
        if (action == "cheat") show_overlay("Kestral Cheat Sheet", CHEAT_SHEET, bg_mode);
        if (action == "quit") {
            if (has_active && active && active->modified) {
                bool canc = false;
                std::string ans = prompt_bar("Unsaved changes — save before quit? (y/n/c): ", "", nullptr, bg_mode, &canc);
                if (canc || (!ans.empty() && (ans[0] == 'c' || ans[0] == 'C'))) continue;
                if (!ans.empty() && (ans[0] == 'y' || ans[0] == 'Y')) {
                    if (active->filepath.empty()) {
                        std::string fn = prompt_bar("Save as: ", "untitled.kes", nullptr, bg_mode, &canc);
                        if (canc || fn.empty()) continue;
                        active->save(fn);
                    } else {
                        active->save();
                    }
                }
            }
            return false;
        }
    }
}

int run_nana(const std::string& filepath, const std::string& bg_override) {
    if (!stdin_stdout_are_tty()) {
        std::cerr << "nana requires an interactive terminal.\n";
        return 1;
    }
    std::string bg_mode = bg_override.empty() ? load_bg_mode() : save_bg_mode(bg_override);
    RawTtyGuard tty;
    tty.enter();

    std::cout << "\x1b[?1049h\x1b[?25h" << std::flush;
    struct ScreenRestore {
        ~ScreenRestore() { std::cout << "\x1b[?1049l\x1b[?25h\x1b[0m" << std::flush; }
    } screen_restore;

    NanaBuffer buf;
    bool has_buf = !filepath.empty();
    if (has_buf) {
        buf = NanaBuffer::from_file(filepath);
    } else {
        if (!run_menu_tui(nullptr, has_buf, bg_mode, buf)) return 0;
        has_buf = true;
    }

    for (;;) {
        auto [rows, cols] = get_terminal_size();
        std::cout << buf.render_frame(rows, cols, bg_mode) << std::flush;
        std::string k = read_key();
        if (k == "RESIZE") continue;
        buf.status.clear();

        if (k == "\x10") {
            NanaBuffer next_buf;
            if (!run_menu_tui(&buf, has_buf, bg_mode, next_buf)) return 0;
            buf = std::move(next_buf);
            continue;
        }
        if (k == "\x18" || k == "\x11") {
            if (buf.modified) {
                bool canc = false;
                std::string ans = prompt_bar("Save modified buffer? (y/n/c): ", "", &buf, bg_mode, &canc);
                if (canc || (!ans.empty() && (ans[0] == 'c' || ans[0] == 'C'))) continue;
                if (!ans.empty() && (ans[0] == 'y' || ans[0] == 'Y')) {
                    if (buf.filepath.empty()) {
                        std::string fn = prompt_bar("Save as: ", "untitled.kes", &buf, bg_mode, &canc);
                        if (canc || fn.empty()) continue;
                        buf.save(fn);
                    } else {
                        buf.save();
                    }
                }
            }
            return 0;
        }
        if (k == "\x13" || k == "\x0f") {
            if (buf.filepath.empty()) {
                bool canc = false;
                std::string fn = prompt_bar("Save as: ", "untitled.kes", &buf, bg_mode, &canc);
                if (!canc && !fn.empty()) buf.save(fn);
            } else {
                buf.save();
            }
            continue;
        }
        if (k == "\x12") { run_buffer_in_nana(buf, bg_mode); continue; }
        if (k == "\x07") { show_overlay("Kestral & nana Help", CHEAT_SHEET, bg_mode); continue; }
        if (k == "\x0b") { buf.cut_line(); continue; }
        if (k == "\x15") { buf.paste_line(); continue; }
        if (k == "UP") buf.move_up();
        else if (k == "DOWN") buf.move_down();
        else if (k == "LEFT") buf.move_left();
        else if (k == "RIGHT") buf.move_right();
        else if (k == "HOME") buf.move_home();
        else if (k == "END") buf.move_end();
        else if (k == "DELETE") buf.del();
        else if (k == "\x7f" || k == "\x08") buf.backspace();
        else if (k == "\r" || k == "\n") buf.insert_newline();
        else if (k == "\t") buf.insert_tab();
        else if (k.size() == 1 && static_cast<unsigned char>(k[0]) >= 32 && k[0] != 127) {
            buf.insert_char(k[0]);
        }
    }
}

} // namespace kestral
