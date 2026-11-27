#pragma once

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace kestral {

struct KestralError : public std::runtime_error {
    int line = 0;
    int col = 0;
    std::string phase = "runtime"; // "syntax" | "runtime"
    bool incomplete = false;
    bool fatal = false;

    KestralError(std::string msg, int line_ = 0, int col_ = 0,
                 std::string phase_ = "runtime", bool incomplete_ = false,
                 bool fatal_ = false)
        : std::runtime_error(std::move(msg)),
          line(line_),
          col(col_),
          phase(std::move(phase_)),
          incomplete(incomplete_),
          fatal(fatal_) {}
};

inline std::string to_lower_ascii(std::string s) {
    for (char& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

inline int levenshtein_distance(const std::string& a, const std::string& b) {
    const std::size_t n = a.size(), m = b.size();
    std::vector<std::vector<int>> dp(n + 1, std::vector<int>(m + 1, 0));
    for (std::size_t i = 0; i <= n; ++i) dp[i][0] = static_cast<int>(i);
    for (std::size_t j = 0; j <= m; ++j) dp[0][j] = static_cast<int>(j);
    for (std::size_t i = 1; i <= n; ++i) {
        for (std::size_t j = 1; j <= m; ++j) {
            int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            int d = std::min({dp[i - 1][j] + 1, dp[i][j - 1] + 1, dp[i - 1][j - 1] + cost});
            if (i > 1 && j > 1 && a[i - 1] == b[j - 2] && a[i - 2] == b[j - 1]) {
                d = std::min(d, dp[i - 2][j - 2] + 1);
            }
            dp[i][j] = d;
        }
    }
    return dp[n][m];
}

inline std::string did_you_mean(const std::string& target,
                                const std::vector<std::string>& candidates) {
    if (target.empty()) return "";
    std::string best;
    int best_dist = 999;
    std::string tlow = to_lower_ascii(target);
    int max_dist = (target.size() <= 3) ? 1 : 2;
    for (const auto& c : candidates) {
        if (c == target) continue;
        int d = levenshtein_distance(tlow, to_lower_ascii(c));
        if (d <= max_dist && d < best_dist) {
            best_dist = d;
            best = c;
        }
    }
    return best;
}

inline std::string format_error(const KestralError& err, const std::string& src) {
    std::ostringstream out;
    std::string kind = (err.phase == "syntax") ? "Syntax error" : "Error";
    if (err.line > 0) {
        out << kind << " on line " << err.line;
        if (err.col > 0) out << ", column " << err.col;
        out << ": " << err.what();
        std::vector<std::string> lines;
        std::istringstream ss(src);
        std::string ln;
        while (std::getline(ss, ln)) {
            if (!ln.empty() && ln.back() == '\r') ln.pop_back();
            lines.push_back(ln);
        }
        if (err.line >= 1 && static_cast<std::size_t>(err.line) <= lines.size()) {
            const std::string& src_line = lines[static_cast<std::size_t>(err.line - 1)];
            std::string lnum = std::to_string(err.line);
            out << "\n" << lnum << " | " << src_line;
            if (err.col > 0) {
                std::string pad(lnum.size() + 3 + static_cast<std::size_t>(err.col - 1), ' ');
                out << "\n" << pad << "^";
            }
        }
    } else {
        out << kind << ": " << err.what();
    }
    return out.str();
}

} // namespace kestral
