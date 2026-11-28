#include "banner.hpp"
#include "bundler.hpp"
#include "error.hpp"
#include "interp.hpp"
#include "learn.hpp"
#include "nana.hpp"
#include "parser.hpp"
#include "repl.hpp"
#include "version.hpp"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#include <process.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace {

std::string read_text_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Could not open \"" + path + "\".");
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

void print_help() {
    std::cout <<
        "Kestral " << KESTRAL_VERSION << " — fast, friendly general-purpose CLI language (.kes)\n\n"
        "Usage:\n"
        "  kestral                              Start the interactive REPL\n"
        "  kestral <file.kes> [args...]         Run a Kestral program\n"
        "  kestral run <file.kes>               Run a Kestral program\n"
        "  kestral build <file.kes> [-o <out>]  Compile a .kes program into a standalone binary\n"
        "  kestral compile <file.kes> [-o <out>] Alias for build (--run executes after build)\n"
        "  kestral -e \"<code>\"                  Run inline Kestral code\n"
        "  kestral check <file.kes>             Check syntax without running\n"
        "  kestral nana [file.kes]              Open the minimalist nana TUI editor\n"
        "  kestral learn [n]                    Launch the interactive Kestral TUI course\n"
        "  kestral banner                       Print the Kestral startup card\n"
        "  kestral --version | --help           Show version or help\n\n"
        "TUI Surface Flags:\n"
        "  --bg mica | black            Use transparent Mica (default) or pure Black (#000000)\n";
}

int exec_built_binary(const std::filesystem::path& bin_path) {
    std::string abs = std::filesystem::absolute(bin_path).string();
    std::vector<char> arg0(abs.begin(), abs.end());
    arg0.push_back('\0');
    char* const child_argv[] = {arg0.data(), nullptr};
#ifdef _WIN32
    intptr_t rc = _spawnv(_P_WAIT, abs.c_str(), child_argv);
    return (rc < 0) ? 1 : static_cast<int>(rc);
#else
    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "Could not start compiled binary.\n";
        return 1;
    }
    if (pid == 0) {
        execv(abs.c_str(), child_argv);
        _exit(127);
    }
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) return 1;
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return 1;
#endif
}

int run_embedded_bundle(const kestral::EmbeddedBundle& bundle,
                        const std::vector<std::string>& script_args) {
    std::unordered_map<std::string, std::string> bundled_map;
    for (const auto& kv : bundle.files) {
        bundled_map[kv.first] = kv.second;
    }
    kestral::InterpConfig cfg;
    cfg.cli_args = script_args;
    cfg.read_file_fn = [bundled_map](const std::string& rel) {
        std::string norm = std::filesystem::path(rel).generic_string();
        auto it = bundled_map.find(norm);
        if (it != bundled_map.end()) return it->second;
        auto it_raw = bundled_map.find(rel);
        if (it_raw != bundled_map.end()) return it_raw->second;
        return read_text_file(rel);
    };
    try {
        kestral::Interp interp(std::move(cfg));
        interp.run(bundle.entry_source);
        return 0;
    } catch (const kestral::KestralError& err) {
        std::cerr << kestral::format_error(err, bundle.entry_source) << '\n';
        return 1;
    }
}

int run_file(const std::string& file, const std::vector<std::string>& script_args) {
    std::string src;
    try {
        src = read_text_file(file);
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 2;
    }
    auto base_dir = std::filesystem::absolute(file).parent_path();
    kestral::InterpConfig cfg;
    cfg.cli_args = script_args;
    cfg.read_file_fn = [base_dir](const std::string& rel) {
        return read_text_file((base_dir / rel).string());
    };
    try {
        kestral::Interp interp(std::move(cfg));
        interp.run(src);
        return 0;
    } catch (const kestral::KestralError& err) {
        std::cerr << kestral::format_error(err, src) << '\n';
        return 1;
    }
}

} // namespace

int main(int argc, char** argv) {
    kestral::enable_utf8_console();
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) args.emplace_back(argv[i]);

    if (auto bundle = kestral::read_embedded_bundle()) {
        return run_embedded_bundle(*bundle, args);
    }

    if (args.empty()) return kestral::run_repl();

    const std::string& cmd = args[0];
    if (cmd == "--help" || cmd == "-h") {
        print_help();
        return 0;
    }
    if (cmd == "--version" || cmd == "-v") {
        std::cout << "kestral " << KESTRAL_VERSION << '\n';
        return 0;
    }
    if (cmd == "repl") {
        return kestral::run_repl();
    }
    if (cmd == "banner") {
        std::cout << kestral::render_startup_banner(KESTRAL_VERSION, true, true) << '\n';
        return 0;
    }
    if (cmd == "-e") {
        if (args.size() < 2) {
            std::cerr << "Give some Kestral code after -e.\n";
            return 2;
        }
        kestral::InterpConfig cfg;
        for (std::size_t i = 2; i < args.size(); ++i) cfg.cli_args.push_back(args[i]);
        cfg.read_file_fn = [](const std::string& rel) { return read_text_file(rel); };
        try {
            kestral::Interp interp(std::move(cfg));
            interp.run(args[1]);
            return 0;
        } catch (const kestral::KestralError& err) {
            std::cerr << kestral::format_error(err, args[1]) << '\n';
            return 1;
        }
    }
    if (cmd == "check") {
        if (args.size() < 2) {
            std::cerr << "Usage: kestral check <file.kes>\n";
            return 2;
        }
        std::string src;
        try {
            src = read_text_file(args[1]);
        } catch (const std::exception& e) {
            std::cerr << e.what() << '\n';
            return 2;
        }
        try {
            kestral::parse(src);
            std::cout << "OK: " << args[1] << '\n';
            return 0;
        } catch (const kestral::KestralError& err) {
            std::cerr << kestral::format_error(err, src) << '\n';
            return 1;
        }
    }
    if (cmd == "nana") {
        std::string target;
        std::string bg;
        for (std::size_t i = 1; i < args.size(); ++i) {
            if (args[i] == "--bg" && i + 1 < args.size()) {
                bg = args[++i];
            } else if (!args[i].empty() && args[i][0] != '-') {
                target = args[i];
            }
        }
        return kestral::run_nana(target, bg);
    }
    if (cmd == "learn") {
        int lesson = 0;
        bool list_only = false;
        bool reset = false;
        std::string bg;
        for (std::size_t i = 1; i < args.size(); ++i) {
            if (args[i] == "--list") list_only = true;
            else if (args[i] == "--reset") reset = true;
            else if (args[i] == "--bg" && i + 1 < args.size()) bg = args[++i];
            else if (!args[i].empty() && std::isdigit(static_cast<unsigned char>(args[i][0]))) {
                lesson = std::stoi(args[i]);
            }
        }
        return kestral::run_learn(lesson, list_only, reset, bg, false);
    }
    if (cmd == "build" || cmd == "compile") {
        std::string source_file;
        std::string out_file;
        bool run_after = false;
        for (std::size_t i = 1; i < args.size(); ++i) {
            if ((args[i] == "-o" || args[i] == "--output") && i + 1 < args.size()) {
                out_file = args[++i];
            } else if (args[i] == "--run") {
                run_after = true;
            } else if (args[i] == "--help" || args[i] == "-h") {
                std::cout << "Usage: kestral " << cmd << " <file.kes> [-o <out>] [--run]\n"
                          << "Compiles a Kestral program into a standalone native binary.\n";
                return 0;
            } else if (!args[i].empty() && args[i][0] == '-') {
                std::cerr << "Unknown option for " << cmd << ": " << args[i] << '\n';
                return 2;
            } else if (source_file.empty()) {
                source_file = args[i];
            } else {
                std::cerr << "Unexpected argument: " << args[i] << '\n';
                return 2;
            }
        }
        if (source_file.empty()) {
            std::cerr << "Usage: kestral " << cmd << " <file.kes> [-o <out>] [--run]\n";
            return 2;
        }
        std::string src_preview;
        try {
            src_preview = read_text_file(source_file);
            auto res = kestral::build_standalone_binary(source_file, out_file);
            std::size_t kb = (res.size_bytes + 1023) / 1024;
            std::cout << "Built standalone binary: " << res.output_path.string() << " (" << kb << " KB)\n";
#ifdef _WIN32
            std::string run_hint = res.output_path.string();
#else
            std::string run_hint = res.output_path.has_parent_path()
                                       ? res.output_path.string()
                                       : ("./" + res.output_path.string());
#endif
            std::cout << "Run it with: " << run_hint << '\n';
            if (run_after) {
                return exec_built_binary(res.output_path);
            }
            return 0;
        } catch (const kestral::KestralError& err) {
            std::cerr << kestral::format_error(err, src_preview) << '\n';
            return 1;
        } catch (const std::exception& e) {
            std::cerr << e.what() << '\n';
            return 1;
        }
    }
    if (cmd == "run") {
        if (args.size() < 2) {
            std::cerr << "Usage: kestral run <file.kes> [args...]\n";
            return 2;
        }
        std::vector<std::string> rest(args.begin() + 2, args.end());
        return run_file(args[1], rest);
    }

    std::vector<std::string> rest(args.begin() + 1, args.end());
    return run_file(args[0], rest);
}
