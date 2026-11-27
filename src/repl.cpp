#include "repl.hpp"
#include "banner.hpp"
#include "error.hpp"
#include "interp.hpp"
#include "learn.hpp"
#include "nana.hpp"
#include "version.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace kestral {

namespace {

Interp make_repl_interp(const std::filesystem::path& base_dir) {
    InterpConfig cfg;
    cfg.read_file_fn = [base_dir](const std::string& p) -> std::string {
        std::ifstream f(base_dir / p, std::ios::binary);
        if (!f) throw std::runtime_error("file not found");
        std::ostringstream ss;
        ss << f.rdbuf();
        return ss.str();
    };
    return Interp(std::move(cfg));
}

std::string trim_ws(const std::string& s) {
    std::size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r')) a++;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r')) b--;
    return s.substr(a, b - a);
}

void print_repl_help() {
    std::cout <<
        "Kestral REPL commands:\n"
        "  .help            Show this help message\n"
        "  .nana [file]     Open the built-in nana TUI editor\n"
        "  .learn [n]       Launch the interactive Kestral TUI course\n"
        "  .vars            List defined variables and functions\n"
        "  .run <file.kes>  Run a .kes file inside the current session\n"
        "  .reset           Reset workspace state (preserves built-ins)\n"
        "  .quit            Exit the REPL (also .exit)\n";
}

} // namespace

int run_repl() {
    enable_utf8_console();
    std::cout << render_startup_banner(KESTRAL_VERSION, true, true) << '\n';

    auto cwd = std::filesystem::current_path();
    Interp interp = make_repl_interp(cwd);
    std::string buffer;
    std::string line;

    for (;;) {
        std::cout << (buffer.empty() ? "kes> " : "...  ") << std::flush;
        if (!std::getline(std::cin, line)) {
            if (!buffer.empty()) {
                try {
                    Value v = interp.run(buffer);
                    if (interp.last_was_expr && !v.is_nothing()) {
                        std::cout << format_value(v, true) << '\n';
                    }
                } catch (const KestralError& err) {
                    std::cerr << format_error(err, buffer) << '\n';
                }
            }
            std::cout << '\n';
            return 0;
        }
        if (!line.empty() && line.back() == '\r') line.pop_back();

        if (buffer.empty()) {
            std::string tr = trim_ws(line);
            if (tr.empty()) continue;

            std::string cmd = tr;
            std::string arg;
            auto sp = tr.find(' ');
            if (sp != std::string::npos) {
                cmd = tr.substr(0, sp);
                arg = trim_ws(tr.substr(sp + 1));
            }
            std::string low = to_lower_ascii(cmd);

            if (low == ".exit" || low == ".quit" || low == "quit" || low == "exit") {
                return 0;
            }
            if (low == ".help" || low == "help") {
                print_repl_help();
                continue;
            }
            if (low == ".clear" || low == ".reset") {
                interp = make_repl_interp(cwd);
                std::cout << "Workspace reset (all built-ins ready).\n";
                continue;
            }
            if (low == ".vars" || low == "vars") {
                auto names = interp.defined_var_names();
                if (names.empty()) {
                    std::cout << "(no user variables defined)\n";
                } else {
                    for (const auto& nm : names) {
                        std::cout << "  " << nm << " = "
                                  << format_value(interp.global_env->vars.at(nm), true) << '\n';
                    }
                }
                continue;
            }
            if (low == ".nana" || (low == "nana" && !interp.global_env->has_local("nana"))) {
                run_nana(arg);
                continue;
            }
            if (low == ".learn" || (low == "learn" && !interp.global_env->has_local("learn"))) {
                if (arg == "--list") {
                    run_learn(0, true, false);
                } else if (!arg.empty()) {
                    try {
                        int n = std::stoi(arg);
                        run_learn(n, false, false);
                    } catch (...) {
                        std::cout << "Usage: .learn [lesson_number | --list]\n";
                    }
                } else {
                    run_learn(0, false, false);
                }
                continue;
            }
            if (low == ".run") {
                if (arg.empty()) {
                    std::cout << "Usage: .run <file.kes>\n";
                    continue;
                }
                std::ifstream f(arg, std::ios::binary);
                if (!f) {
                    std::cerr << "Could not open \"" << arg << "\".\n";
                    continue;
                }
                std::ostringstream ss;
                ss << f.rdbuf();
                std::string fsrc = ss.str();
                try {
                    Value v = interp.run(fsrc);
                    if (interp.last_was_expr && !v.is_nothing()) {
                        std::cout << format_value(v, true) << '\n';
                    }
                } catch (const KestralError& err) {
                    std::cerr << format_error(err, fsrc) << '\n';
                }
                continue;
            }
        }

        buffer += (buffer.empty() ? "" : "\n") + line;
        try {
            Value v = interp.run(buffer);
            if (interp.last_was_expr && !v.is_nothing()) {
                std::cout << format_value(v, true) << '\n';
            }
            buffer.clear();
        } catch (const KestralError& err) {
            if (err.incomplete) continue;
            std::cerr << format_error(err, buffer) << '\n';
            buffer.clear();
        }
    }
}

} // namespace kestral
