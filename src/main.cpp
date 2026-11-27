#include "banner.hpp"
#include "error.hpp"
#include "interp.hpp"
#include "learn.hpp"
#include "nana.hpp"
#include "parser.hpp"
#include "repl.hpp"
#include "version.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

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
        "  kestral                      Start the interactive REPL\n"
        "  kestral <file.kes> [args...] Run a Kestral program\n"
        "  kestral run <file.kes>       Run a Kestral program\n"
        "  kestral -e \"<code>\"          Run inline Kestral code\n"
        "  kestral check <file.kes>     Check syntax without running\n"
        "  kestral nana [file.kes]      Open the minimalist nana TUI editor\n"
        "  kestral learn [n]            Launch the interactive Kestral TUI course\n"
        "  kestral banner               Print the Kestral startup card\n"
        "  kestral --version | --help   Show version or help\n\n"
        "TUI Surface Flags:\n"
        "  --bg mica | black            Use transparent Mica (default) or pure Black (#000000)\n";
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
