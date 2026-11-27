#pragma once

#include <algorithm>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace kestral {

inline void enable_utf8_console() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD mode = 0;
        if (GetConsoleMode(hOut, &mode)) {
            SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
    }
#endif
}

inline std::string render_startup_banner(const std::string& version,
                                         bool color = true,
                                         bool unicode_ok = true) {
    auto dim = [&](const std::string& s) {
        return color ? ("\x1b[2m" + s + "\x1b[0m") : s;
    };
    auto bold = [&](const std::string& s) {
        return color ? ("\x1b[1m" + s + "\x1b[0m") : s;
    };

    if (unicode_ok) {
        const std::vector<std::string> icon = {
            "╭─────────────╮",
            "│ ╷ ╷ ╭─╴ ╭─╮ │",
            "│ ├─╯ ├─╴ ╰─╮ │",
            "│ ╵ ╵ ╰─╴ ╰─╯ │",
            "╰─────────────╯",
        };
        const std::vector<std::pair<std::string, std::string>> right = {
            {"Kestral  v" + version, "bold"},
            {"native C++20 · .kes", "dim"},
            {"", ""},
            {".help  .learn  .nana", "dim"},
            {".vars  .reset  .quit", "dim"},
        };
        const std::vector<std::size_t> vis_icon = {15, 15, 15, 15, 15};
        auto cp_len = [](const std::string& s) {
            std::size_t n = 0;
            for (unsigned char c : s) if ((c & 0xC0) != 0x80) n++;
            return n;
        };
        const std::size_t inner_w = 46;
        std::string hbar;
        for (std::size_t i = 0; i < inner_w; ++i) hbar += "─";

        std::string out = dim("╭" + hbar + "╮") + "\n";
        for (std::size_t i = 0; i < icon.size(); ++i) {
            std::string rs = right[i].first;
            if (right[i].second == "bold") rs = bold(rs);
            else if (right[i].second == "dim") rs = dim(rs);
            std::size_t vis = 2 + vis_icon[i] + 3 + cp_len(right[i].first);
            std::size_t pad = (inner_w > vis) ? (inner_w - vis) : 1;
            out += dim("│") + "  " + bold(icon[i]) + "   " + rs +
                   std::string(pad, ' ') + dim("│") + "\n";
        }
        out += dim("╰" + hbar + "╯");
        return out;
    }

    const std::vector<std::string> icon_a = {
        "+-------------+",
        "| K  E  S     |",
        "|  [ KESTRAL ]|",
        "+-------------+",
    };
    const std::vector<std::pair<std::string, std::string>> right_a = {
        {"Kestral  v" + version, "bold"},
        {"native C++20 .kes", "dim"},
        {".help  .learn  .nana", "dim"},
        {".vars  .reset  .quit", "dim"},
    };
    const std::size_t inner_w = 46;
    std::string out = dim("+" + std::string(inner_w, '-') + "+") + "\n";
    for (std::size_t i = 0; i < icon_a.size(); ++i) {
        std::string rs = right_a[i].first;
        if (right_a[i].second == "bold") rs = bold(rs);
        else if (right_a[i].second == "dim") rs = dim(rs);
        std::size_t vis = 2 + icon_a[i].size() + 3 + right_a[i].first.size();
        std::size_t pad = (inner_w > vis) ? (inner_w - vis) : 1;
        out += dim("|") + "  " + bold(icon_a[i]) + "   " + rs +
               std::string(pad, ' ') + dim("|") + "\n";
    }
    out += dim("+" + std::string(inner_w, '-') + "+");
    return out;
}

} // namespace kestral
