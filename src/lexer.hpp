#pragma once

#include <string>
#include <vector>

namespace kestral {

enum class TokType {
    Num,
    Str,
    InterpStr,
    Id,
    Kw,
    Op,
    Nl,
    Eof
};

struct InterpPart {
    bool is_expr = false;
    std::string text;
    int line = 0;
    int col = 0;
};

struct Token {
    TokType type = TokType::Eof;
    std::string sval;
    double dval = 0.0;
    int line = 1;
    int col = 1;
    std::vector<InterpPart> parts;
};

bool is_keyword(const std::string& w);
std::vector<Token> lex(const std::string& src, int line0 = 1, int col0 = 1);

} // namespace kestral
