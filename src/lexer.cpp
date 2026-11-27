#include "lexer.hpp"
#include "error.hpp"

#include <cctype>
#include <cstdlib>
#include <unordered_set>

namespace kestral {

static const std::unordered_set<std::string>& keywords_set() {
    static const std::unordered_set<std::string> kws = {
        "let", "fn", "if", "else", "while", "for", "in", "loop", "times",
        "stop", "skip", "give", "try", "catch", "fail", "match", "and",
        "or", "not", "true", "false", "nothing", "say", "use"
    };
    return kws;
}

bool is_keyword(const std::string& w) {
    return keywords_set().find(w) != keywords_set().end();
}

static bool is_digit_ch(char c) {
    return c >= '0' && c <= '9';
}

static bool is_id_start(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_id_part(char c) {
    return is_id_start(c) || is_digit_ch(c);
}

std::vector<Token> lex(const std::string& src, int line0, int col0) {
    std::vector<Token> out;
    std::size_t i = 0;
    const std::size_t n = src.size();
    int line = line0;
    std::size_t ls = 0;

    auto cur_col = [&](std::size_t pos) -> int {
        int base = (line == line0) ? col0 : 1;
        return static_cast<int>(pos - ls) + base;
    };

    auto syn_err = [&](const std::string& msg, std::size_t pos, bool inc = false) -> void {
        throw KestralError(msg, line, cur_col(pos), "syntax", inc);
    };

    while (i < n) {
        char c = src[i];
        if (c == ' ' || c == '\t' || c == '\r') { i++; continue; }
        if (c == '\n') {
            Token t;
            t.type = TokType::Nl;
            t.sval = "\n";
            t.line = line;
            t.col = cur_col(i);
            out.push_back(std::move(t));
            i++;
            line++;
            ls = i;
            continue;
        }
        if (c == ';') {
            Token t;
            t.type = TokType::Nl;
            t.sval = ";";
            t.line = line;
            t.col = cur_col(i);
            out.push_back(std::move(t));
            i++;
            continue;
        }
        if (c == '/' && i + 1 < n && src[i + 1] == '/') {
            while (i < n && src[i] != '\n') i++;
            continue;
        }

        if (is_digit_ch(c)) {
            std::size_t s = i;
            while (i < n && is_digit_ch(src[i])) i++;
            if (i + 1 < n && src[i] == '.' && is_digit_ch(src[i + 1])) {
                i++;
                while (i < n && is_digit_ch(src[i])) i++;
            }
            if (i < n && is_id_start(src[i])) {
                syn_err("A number can't be followed by letters (\"" +
                            src.substr(s, i - s + 1) + "...\").",
                        s);
            }
            Token t;
            t.type = TokType::Num;
            t.sval = src.substr(s, i - s);
            t.dval = std::strtod(t.sval.c_str(), nullptr);
            t.line = line;
            t.col = cur_col(s);
            out.push_back(std::move(t));
            continue;
        }

        if (is_id_start(c)) {
            std::size_t s = i;
            while (i < n && is_id_part(src[i])) i++;
            std::string word = src.substr(s, i - s);
            Token t;
            t.type = is_keyword(word) ? TokType::Kw : TokType::Id;
            t.sval = std::move(word);
            t.line = line;
            t.col = cur_col(s);
            out.push_back(std::move(t));
            continue;
        }

        if (c == '"' || c == '\'') {
            std::size_t s = i;
            char q = c;
            int start_line = line;
            int start_col = cur_col(s);
            i++;
            std::vector<InterpPart> parts;
            std::string cur;
            for (;;) {
                if (i >= n || src[i] == '\n') {
                    throw KestralError(
                        std::string("This text never closes. Add a closing ") + q + " at the end.",
                        start_line, start_col, "syntax", false);
                }
                char ch = src[i];
                if (ch == q) {
                    i++;
                    break;
                }
                if (ch == '\\') {
                    if (i + 1 < n) {
                        char e = src[i + 1];
                        if (e == 'n') { cur += '\n'; i += 2; continue; }
                        if (e == 't') { cur += '\t'; i += 2; continue; }
                        if (e == '\\') { cur += '\\'; i += 2; continue; }
                        if (e == '"') { cur += '"'; i += 2; continue; }
                        if (e == '\'') { cur += '\''; i += 2; continue; }
                        if (e == '{') { cur += '{'; i += 2; continue; }
                        if (e == '}') { cur += '}'; i += 2; continue; }
                    }
                    cur += '\\';
                    i++;
                    continue;
                }
                if (ch == '{') {
                    if (!cur.empty()) {
                        parts.push_back(InterpPart{false, cur, line, cur_col(i)});
                        cur.clear();
                    }
                    i++;
                    int expr_start_col = cur_col(i);
                    int depth = 1;
                    std::string code;
                    while (i < n && depth > 0) {
                        char sc = src[i];
                        if (sc == '\n') break;
                        if (sc == '"' || sc == '\'') {
                            char iq = sc;
                            code += iq;
                            i++;
                            while (i < n && src[i] != iq && src[i] != '\n') {
                                if (src[i] == '\\' && i + 1 < n) {
                                    code += src[i];
                                    code += src[i + 1];
                                    i += 2;
                                } else {
                                    code += src[i++];
                                }
                            }
                            if (i < n && src[i] == iq) code += src[i++];
                            continue;
                        }
                        if (sc == '{') depth++;
                        else if (sc == '}') {
                            depth--;
                            if (depth == 0) { i++; break; }
                        }
                        code += sc;
                        i++;
                    }
                    if (depth > 0) {
                        throw KestralError(
                            "Missing } inside {...} text interpolation.",
                            start_line, expr_start_col, "syntax");
                    }
                    parts.push_back(InterpPart{true, code, line, expr_start_col});
                    continue;
                }
                cur += ch;
                i++;
            }
            if (parts.empty()) {
                Token t;
                t.type = TokType::Str;
                t.sval = std::move(cur);
                t.line = start_line;
                t.col = start_col;
                out.push_back(std::move(t));
            } else {
                if (!cur.empty()) {
                    parts.push_back(InterpPart{false, cur, line, cur_col(i)});
                }
                Token t;
                t.type = TokType::InterpStr;
                t.line = start_line;
                t.col = start_col;
                t.parts = std::move(parts);
                out.push_back(std::move(t));
            }
            continue;
        }

        if (i + 1 < n) {
            std::string two = src.substr(i, 2);
            if (two == "==" || two == "!=" || two == "<=" || two == ">=" ||
                two == "+=" || two == "-=" || two == "*=" || two == "/=" ||
                two == "%=" || two == ".." || two == "=>" || two == "|>" ||
                two == "??" || two == "**") {
                Token t;
                t.type = TokType::Op;
                t.sval = std::move(two);
                t.line = line;
                t.col = cur_col(i);
                out.push_back(std::move(t));
                i += 2;
                continue;
            }
            if (two == "&&") {
                syn_err("Kestral writes \"and\" instead of \"&&\".", i);
            }
            if (two == "||") {
                syn_err("Kestral writes \"or\" instead of \"||\".", i);
            }
        }
        if (c == '!') {
            syn_err("Kestral writes \"not\" instead of \"!\".", i);
        }

        static const std::string one_char = "+-*/%<>=()[]{},.:";
        if (one_char.find(c) != std::string::npos) {
            Token t;
            t.type = TokType::Op;
            t.sval = std::string(1, c);
            t.line = line;
            t.col = cur_col(i);
            out.push_back(std::move(t));
            i++;
            continue;
        }

        syn_err(std::string("I don't understand \"") + c + "\" here.", i);
    }

    Token eof;
    eof.type = TokType::Eof;
    eof.sval = "";
    eof.line = line;
    eof.col = cur_col(n);
    out.push_back(std::move(eof));
    return out;
}

} // namespace kestral
