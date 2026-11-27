#pragma once

#include <vector>

#include "ast.hpp"
#include "lexer.hpp"

namespace kestral {

Program parse(const std::string& src);
ExprPtr parse_single_expr_from_tokens(const std::vector<Token>& tokens);

} // namespace kestral
