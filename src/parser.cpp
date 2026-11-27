#include "parser.hpp"
#include "error.hpp"
#include "value.hpp"

#include <unordered_set>

namespace kestral {

namespace {

const std::vector<std::string>& stmt_keywords() {
    static const std::vector<std::string> kws = {
        "let", "fn", "if", "else", "while", "for", "loop", "stop",
        "skip", "give", "try", "catch", "fail", "match", "say", "use"
    };
    return kws;
}

class Parser {
public:
    explicit Parser(const std::vector<Token>& toks) : toks_(toks) {}

    Program parse_program() {
        Program prog;
        skip_nl();
        while (!at(TokType::Eof)) {
            prog.body.push_back(parse_stmt());
            expect_sep();
            skip_nl();
        }
        return prog;
    }

    ExprPtr parse_expression_only() {
        skip_nl();
        if (at(TokType::Eof)) {
            throw KestralError("Empty expression.", peek().line, peek().col, "syntax");
        }
        ExprPtr e = parse_expr();
        skip_nl();
        if (!at(TokType::Eof)) {
            throw KestralError("Unexpected \"" + peek().sval + "\" inside text {...}.",
                               peek().line, peek().col, "syntax");
        }
        return e;
    }

private:
    const std::vector<Token>& toks_;
    std::size_t pos_ = 0;

    const Token& peek(std::size_t off = 0) const {
        std::size_t idx = pos_ + off;
        if (idx >= toks_.size()) return toks_.back();
        return toks_[idx];
    }

    const Token& advance() {
        const Token& t = peek();
        if (pos_ + 1 < toks_.size()) pos_++;
        return t;
    }

    bool at(TokType ty, std::size_t off = 0) const { return peek(off).type == ty; }
    bool at_kw(const std::string& w, std::size_t off = 0) const {
        const Token& t = peek(off);
        return t.type == TokType::Kw && t.sval == w;
    }
    bool at_op(const std::string& op, std::size_t off = 0) const {
        const Token& t = peek(off);
        return t.type == TokType::Op && t.sval == op;
    }
    bool at_id_or_kw(std::size_t off = 0) const {
        const Token& t = peek(off);
        return t.type == TokType::Id || t.type == TokType::Kw;
    }

    void skip_nl() {
        while (at(TokType::Nl)) advance();
    }

    void skip_real_nl_only() {
        while (at(TokType::Nl) && peek().sval == "\n") advance();
    }

    const Token& expect_op(const std::string& op, const std::string& msg = "") {
        if (at_op(op)) return advance();
        const Token& t = peek();
        bool inc = at(TokType::Eof);
        throw KestralError(msg.empty() ? ("expected \"" + op + "\"") : msg,
                           t.line, t.col, "syntax", inc);
    }

    const Token& expect_kw(const std::string& kw, const std::string& msg = "") {
        if (at_kw(kw)) return advance();
        const Token& t = peek();
        bool inc = at(TokType::Eof);
        throw KestralError(msg.empty() ? ("expected \"" + kw + "\"") : msg,
                           t.line, t.col, "syntax", inc);
    }

    std::string expect_name(const std::string& msg = "expected a name") {
        const Token& t = peek();
        if (t.type == TokType::Id) {
            advance();
            return t.sval;
        }
        throw KestralError(msg, t.line, t.col, "syntax");
    }

    void expect_sep() {
        if (at(TokType::Nl)) {
            advance();
            return;
        }
        if (at(TokType::Eof) || at_op("}")) return;
        const Token& t = peek();
        throw KestralError("Each statement needs its own line (or ; before \"" +
                               t.sval + "\").",
                           t.line, t.col, "syntax");
    }

    std::vector<StmtPtr> parse_block() {
        const Token& open = peek();
        expect_op("{", "expected a { to start this block");
        std::vector<StmtPtr> body;
        skip_nl();
        while (!at_op("}")) {
            if (at(TokType::Eof)) {
                throw KestralError("This { block was never closed with a }.",
                                   open.line, open.col, "syntax", true);
            }
            body.push_back(parse_stmt());
            expect_sep();
            skip_nl();
        }
        advance(); // }
        return body;
    }

    void parse_param_list(std::vector<std::string>& names,
                          std::vector<ExprPtr>& defaults) {
        expect_op("(", "expected ( for parameter list");
        skip_nl();
        while (!at_op(")")) {
            if (at(TokType::Eof)) {
                throw KestralError("Parameter list was never closed with ).",
                                   peek().line, peek().col, "syntax", true);
            }
            names.push_back(expect_name("expected parameter name"));
            skip_nl();
            if (at_op("=")) {
                advance();
                skip_nl();
                defaults.push_back(parse_expr());
            } else {
                defaults.push_back(nullptr);
            }
            skip_nl();
            if (at_op(",")) {
                advance();
                skip_nl();
            } else {
                break;
            }
        }
        expect_op(")", "expected ) after parameters");
    }

    StmtPtr parse_stmt() {
        const Token& t = peek();
        if (at_kw("let")) return parse_let();
        if (at_kw("fn") && peek(1).type == TokType::Id) return parse_fn();
        if (at_kw("say")) return parse_say();
        if (at_kw("give")) return parse_give();
        if (at_kw("stop")) {
            advance();
            auto s = std::make_shared<StopStmt>();
            s->line = t.line;
            s->col = t.col;
            return s;
        }
        if (at_kw("skip")) {
            advance();
            auto s = std::make_shared<SkipStmt>();
            s->line = t.line;
            s->col = t.col;
            return s;
        }
        if (at_kw("while")) return parse_while();
        if (at_kw("loop")) return parse_loop();
        if (at_kw("for")) return parse_for();
        if (at_kw("try")) return parse_try();
        if (at_kw("fail")) return parse_fail();
        if (at_kw("use")) return parse_use();

        // Catch likely misspelled statement starters like `sya "hi"` or `lte x = 5`.
        if (t.type == TokType::Id) {
            std::string hint = did_you_mean(t.sval, stmt_keywords());
            if (!hint.empty()) {
                const Token& nxt = peek(1);
                bool looks_like_stmt = false;
                if (hint == "let" && nxt.type == TokType::Id) looks_like_stmt = true;
                else if (hint == "say" && (nxt.type == TokType::Str || nxt.type == TokType::InterpStr ||
                                           nxt.type == TokType::Num)) looks_like_stmt = true;
                else if ((hint == "while" || hint == "if" || hint == "for" || hint == "loop") &&
                         nxt.type != TokType::Nl && nxt.type != TokType::Eof &&
                         !at_op("=", 1) && !at_op("+=", 1) && !at_op("-=", 1) &&
                         !at_op("*=", 1) && !at_op("/=", 1) && !at_op("%=", 1) &&
                         !at_op("(", 1) && !at_op(".", 1) && !at_op("[", 1)) {
                    looks_like_stmt = true;
                }
                if (looks_like_stmt) {
                    throw KestralError("I don't know \"" + t.sval + "\". Did you mean \"" +
                                           hint + "\"?",
                                       t.line, t.col, "syntax");
                }
            }
        }

        ExprPtr lhs = parse_expr();
        if (at_op("=") || at_op("+=") || at_op("-=") || at_op("*=") ||
            at_op("/=") || at_op("%=")) {
            std::string op = advance().sval;
            if (!dynamic_cast<IdentExpr*>(lhs.get()) &&
                !dynamic_cast<MemberExpr*>(lhs.get()) &&
                !dynamic_cast<IndexExpr*>(lhs.get())) {
                throw KestralError("You can only assign to a name, a list item, or a map field.",
                                   t.line, t.col, "syntax");
            }
            ExprPtr rhs = parse_expr();
            auto s = std::make_shared<AssignStmt>();
            s->line = t.line;
            s->col = t.col;
            s->target = std::move(lhs);
            s->op = std::move(op);
            s->value = std::move(rhs);
            return s;
        }

        auto s = std::make_shared<ExprStmt>();
        s->line = t.line;
        s->col = t.col;
        s->expr = std::move(lhs);
        return s;
    }

    StmtPtr parse_let() {
        const Token& t = advance();
        std::string name = expect_name("expected a name after \"let\"");
        expect_op("=", "expected \"=\" after \"" + name + "\" in let");
        ExprPtr val = parse_expr();
        auto s = std::make_shared<LetStmt>();
        s->line = t.line;
        s->col = t.col;
        s->name = std::move(name);
        s->value = std::move(val);
        return s;
    }

    StmtPtr parse_fn() {
        const Token& t = advance();
        auto s = std::make_shared<FnStmt>();
        s->line = t.line;
        s->col = t.col;
        s->name = expect_name("expected a function name after \"fn\"");
        parse_param_list(s->param_names, s->param_defaults);
        skip_nl();
        if (at_op("=>")) {
            advance();
            skip_nl();
            auto ret = std::make_shared<GiveStmt>();
            ret->line = peek().line;
            ret->col = peek().col;
            ret->value = parse_expr();
            s->body.push_back(std::move(ret));
        } else {
            s->body = parse_block();
        }
        return s;
    }

    StmtPtr parse_say() {
        const Token& t = advance();
        auto s = std::make_shared<SayStmt>();
        s->line = t.line;
        s->col = t.col;
        if (at(TokType::Nl) || at(TokType::Eof) || at_op("}")) return s;
        s->args.push_back(parse_expr());
        while (at_op(",")) {
            advance();
            skip_nl();
            s->args.push_back(parse_expr());
        }
        return s;
    }

    StmtPtr parse_give() {
        const Token& t = advance();
        auto s = std::make_shared<GiveStmt>();
        s->line = t.line;
        s->col = t.col;
        if (!at(TokType::Nl) && !at(TokType::Eof) && !at_op("}")) {
            s->value = parse_expr();
        }
        return s;
    }

    StmtPtr parse_while() {
        const Token& t = advance();
        auto s = std::make_shared<WhileStmt>();
        s->line = t.line;
        s->col = t.col;
        s->cond = parse_expr();
        skip_nl();
        s->body = parse_block();
        return s;
    }

    StmtPtr parse_loop() {
        const Token& t = advance();
        auto s = std::make_shared<LoopStmt>();
        s->line = t.line;
        s->col = t.col;
        s->count = parse_expr();
        expect_kw("times", "expected \"times\" after loop count");
        skip_nl();
        s->body = parse_block();
        return s;
    }

    StmtPtr parse_for() {
        const Token& t = advance();
        auto s = std::make_shared<ForStmt>();
        s->line = t.line;
        s->col = t.col;
        s->var1 = expect_name("expected a loop variable name after \"for\"");
        if (at_op(",")) {
            advance();
            s->var2 = expect_name("expected second loop variable name");
        }
        expect_kw("in", "expected \"in\" inside for loop");
        s->iterable = parse_expr();
        skip_nl();
        s->body = parse_block();
        return s;
    }

    StmtPtr parse_try() {
        const Token& t = advance();
        auto s = std::make_shared<TryStmt>();
        s->line = t.line;
        s->col = t.col;
        skip_nl();
        s->try_body = parse_block();
        skip_nl();
        if (!at_kw("catch")) {
            throw KestralError("This try block needs a catch { ... } after it.",
                               peek().line, peek().col, "syntax", at(TokType::Eof));
        }
        advance();
        if (peek().type == TokType::Id) {
            s->err_var = advance().sval;
        }
        skip_nl();
        s->catch_body = parse_block();
        return s;
    }

    StmtPtr parse_fail() {
        const Token& t = advance();
        auto s = std::make_shared<FailStmt>();
        s->line = t.line;
        s->col = t.col;
        s->value = parse_expr();
        return s;
    }

    StmtPtr parse_use() {
        const Token& t = advance();
        auto s = std::make_shared<UseStmt>();
        s->line = t.line;
        s->col = t.col;
        s->path = parse_expr();
        return s;
    }

    /* ---- Expressions ---- */

    ExprPtr parse_expr() { return parse_pipe(); }

    ExprPtr parse_pipe() {
        ExprPtr e = parse_nullish();
        for (;;) {
            // Allow `|> operator` on next indented line
            std::size_t saved = pos_;
            skip_real_nl_only();
            if (!at_op("|>")) {
                pos_ = saved;
                break;
            }
            const Token& op = advance();
            skip_nl();
            const Token& tgt_tok = peek();
            ExprPtr rhs = parse_nullish();
            auto p = std::make_shared<PipeExpr>();
            p->line = op.line;
            p->col = op.col;
            p->value = std::move(e);
            p->target_line = tgt_tok.line;
            p->target_col = tgt_tok.col;
            if (auto* call = dynamic_cast<CallExpr*>(rhs.get())) {
                p->target_expr = call->callee;
                p->extra_args = call->args;
            } else {
                p->target_expr = rhs;
            }
            e = std::move(p);
        }
        return e;
    }

    ExprPtr parse_nullish() {
        ExprPtr e = parse_or();
        while (at_op("??")) {
            const Token& op = advance();
            skip_nl();
            auto b = std::make_shared<BinaryExpr>();
            b->line = op.line;
            b->col = op.col;
            b->op = "??";
            b->left = std::move(e);
            b->right = parse_or();
            e = std::move(b);
        }
        return e;
    }

    ExprPtr parse_or() {
        ExprPtr e = parse_and();
        while (at_kw("or")) {
            const Token& op = advance();
            skip_nl();
            auto b = std::make_shared<BinaryExpr>();
            b->line = op.line;
            b->col = op.col;
            b->op = "or";
            b->left = std::move(e);
            b->right = parse_and();
            e = std::move(b);
        }
        return e;
    }

    ExprPtr parse_and() {
        ExprPtr e = parse_not();
        while (at_kw("and")) {
            const Token& op = advance();
            skip_nl();
            auto b = std::make_shared<BinaryExpr>();
            b->line = op.line;
            b->col = op.col;
            b->op = "and";
            b->left = std::move(e);
            b->right = parse_not();
            e = std::move(b);
        }
        return e;
    }

    ExprPtr parse_not() {
        if (at_kw("not")) {
            const Token& op = advance();
            skip_nl();
            auto u = std::make_shared<UnaryExpr>();
            u->line = op.line;
            u->col = op.col;
            u->op = "not";
            u->operand = parse_not();
            return u;
        }
        return parse_cmp();
    }

    ExprPtr parse_cmp() {
        ExprPtr e = parse_range();
        while (at_op("==") || at_op("!=") || at_op("<") ||
               at_op("<=") || at_op(">") || at_op(">=")) {
            const Token& op = advance();
            skip_nl();
            auto b = std::make_shared<BinaryExpr>();
            b->line = op.line;
            b->col = op.col;
            b->op = op.sval;
            b->left = std::move(e);
            b->right = parse_range();
            e = std::move(b);
        }
        return e;
    }

    ExprPtr parse_range() {
        ExprPtr e = parse_add();
        if (at_op("..")) {
            const Token& op = advance();
            auto b = std::make_shared<BinaryExpr>();
            b->line = op.line;
            b->col = op.col;
            b->op = "..";
            b->left = std::move(e);
            b->right = parse_add();
            return b;
        }
        return e;
    }

    ExprPtr parse_add() {
        ExprPtr e = parse_mul();
        while (at_op("+") || at_op("-")) {
            const Token& op = advance();
            // Continuation after newline allowed (test: `1 +\n 2`)
            if (at(TokType::Nl) && peek().sval == "\n") skip_nl();
            auto b = std::make_shared<BinaryExpr>();
            b->line = op.line;
            b->col = op.col;
            b->op = op.sval;
            b->left = std::move(e);
            b->right = parse_mul();
            e = std::move(b);
        }
        return e;
    }

    ExprPtr parse_mul() {
        ExprPtr e = parse_unary();
        while (at_op("*") || at_op("/") || at_op("%")) {
            const Token& op = advance();
            if (at(TokType::Nl) && peek().sval == "\n") skip_nl();
            auto b = std::make_shared<BinaryExpr>();
            b->line = op.line;
            b->col = op.col;
            b->op = op.sval;
            b->left = std::move(e);
            b->right = parse_unary();
            e = std::move(b);
        }
        return e;
    }

    ExprPtr parse_unary() {
        if (at_op("-")) {
            const Token& op = advance();
            auto u = std::make_shared<UnaryExpr>();
            u->line = op.line;
            u->col = op.col;
            u->op = "-";
            u->operand = parse_unary();
            return u;
        }
        return parse_pow();
    }

    ExprPtr parse_pow() {
        ExprPtr e = parse_postfix();
        if (at_op("**")) {
            const Token& op = advance();
            auto b = std::make_shared<BinaryExpr>();
            b->line = op.line;
            b->col = op.col;
            b->op = "**";
            b->left = std::move(e);
            b->right = parse_unary(); // right-associative, and -2 ** 2 == -(2 ** 2), 2 ** -1 == 0.5
            return b;
        }
        return e;
    }

    ExprPtr parse_postfix() {
        ExprPtr e = parse_primary();
        for (;;) {
            if (at_op("(")) {
                const Token& op = advance();
                auto c = std::make_shared<CallExpr>();
                c->line = op.line;
                c->col = op.col;
                c->callee = std::move(e);
                skip_nl();
                while (!at_op(")")) {
                    if (at(TokType::Eof)) {
                        throw KestralError("Missing ) to close function call.",
                                           op.line, op.col, "syntax", true);
                    }
                    c->args.push_back(parse_expr());
                    skip_nl();
                    if (at_op(",")) {
                        advance();
                        skip_nl();
                    } else {
                        break;
                    }
                }
                expect_op(")", "expected ) after arguments");
                e = std::move(c);
                continue;
            }
            if (at_op("[")) {
                const Token& op = advance();
                skip_nl();
                ExprPtr idx = parse_expr();
                skip_nl();
                expect_op("]", "expected ] after index");
                auto ix = std::make_shared<IndexExpr>();
                ix->line = op.line;
                ix->col = op.col;
                ix->object = std::move(e);
                ix->index = std::move(idx);
                e = std::move(ix);
                continue;
            }
            if (at_op(".")) {
                const Token& op = advance();
                const Token& mt = peek();
                if (!at_id_or_kw()) {
                    throw KestralError("expected a member name after \".\"",
                                       mt.line, mt.col, "syntax");
                }
                advance();
                auto m = std::make_shared<MemberExpr>();
                m->line = op.line;
                m->col = op.col;
                m->object = std::move(e);
                m->member = mt.sval;
                m->member_line = mt.line;
                m->member_col = mt.col;
                e = std::move(m);
                continue;
            }
            // Method call chained across newline: `"  hi "\n  .trim()`
            std::size_t saved = pos_;
            skip_real_nl_only();
            if (at_op(".")) continue;
            pos_ = saved;
            break;
        }
        return e;
    }

    ExprPtr parse_primary() {
        const Token& t = peek();
        if (at(TokType::Num)) {
            advance();
            auto e = std::make_shared<LiteralExpr>();
            e->line = t.line;
            e->col = t.col;
            e->kind = LiteralExpr::Kind::Number;
            e->dval = t.dval;
            return e;
        }
        if (at(TokType::Str)) {
            advance();
            auto e = std::make_shared<LiteralExpr>();
            e->line = t.line;
            e->col = t.col;
            e->kind = LiteralExpr::Kind::Text;
            e->sval = t.sval;
            return e;
        }
        if (at(TokType::InterpStr)) {
            advance();
            auto e = std::make_shared<InterpTextExpr>();
            e->line = t.line;
            e->col = t.col;
            for (const auto& part : t.parts) {
                InterpTextExpr::Segment seg;
                seg.is_expr = part.is_expr;
                seg.text = part.text;
                if (part.is_expr) {
                    bool all_ws = true;
                    for (char ch : part.text) {
                        if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n') {
                            all_ws = false;
                            break;
                        }
                    }
                    if (all_ws) {
                        throw KestralError("Empty {} inside text.", part.line, part.col, "syntax");
                    }
                    auto sub_toks = lex(part.text, part.line, part.col);
                    seg.expr = parse_single_expr_from_tokens(sub_toks);
                }
                e->segments.push_back(std::move(seg));
            }
            return e;
        }
        if (at_kw("true") || at_kw("false")) {
            advance();
            auto e = std::make_shared<LiteralExpr>();
            e->line = t.line;
            e->col = t.col;
            e->kind = LiteralExpr::Kind::Bool;
            e->bval = (t.sval == "true");
            return e;
        }
        if (at_kw("nothing")) {
            advance();
            auto e = std::make_shared<LiteralExpr>();
            e->line = t.line;
            e->col = t.col;
            e->kind = LiteralExpr::Kind::Nothing;
            return e;
        }
        if (at_kw("fn")) {
            advance();
            auto lam = std::make_shared<LambdaExpr>();
            lam->line = t.line;
            lam->col = t.col;
            parse_param_list(lam->param_names, lam->param_defaults);
            skip_nl();
            if (at_op("=>")) {
                advance();
                skip_nl();
                auto ret = std::make_shared<GiveStmt>();
                ret->line = peek().line;
                ret->col = peek().col;
                ret->value = parse_expr();
                lam->body.push_back(std::move(ret));
            } else {
                lam->body = parse_block();
            }
            return lam;
        }
        if (at_kw("if")) {
            return parse_if_expr();
        }
        if (at_kw("match")) {
            return parse_match_expr();
        }
        if (at(TokType::Id)) {
            advance();
            auto e = std::make_shared<IdentExpr>();
            e->line = t.line;
            e->col = t.col;
            e->name = t.sval;
            return e;
        }
        if (at_op("(")) {
            advance();
            skip_nl();
            ExprPtr inner = parse_expr();
            skip_nl();
            expect_op(")", "expected )");
            return inner;
        }
        if (at_op("[")) {
            return parse_list();
        }
        if (at_op("{")) {
            return parse_map();
        }
        throw KestralError("I didn't expect \"" + (t.sval.empty() ? "end of file" : t.sval) + "\" here.",
                           t.line, t.col, "syntax", at(TokType::Eof));
    }

    ExprPtr parse_list() {
        const Token& open = advance();
        auto list = std::make_shared<ListExpr>();
        list->line = open.line;
        list->col = open.col;
        skip_nl();
        while (!at_op("]")) {
            if (at(TokType::Eof)) {
                throw KestralError("List [ was never closed with ].",
                                   open.line, open.col, "syntax", true);
            }
            list->elements.push_back(parse_expr());
            skip_nl();
            if (at_op(",")) {
                advance();
                skip_nl();
            } else {
                break;
            }
        }
        expect_op("]", "expected ] to close list");
        return list;
    }

    ExprPtr parse_map() {
        const Token& open = advance();
        auto mp = std::make_shared<MapExpr>();
        mp->line = open.line;
        mp->col = open.col;
        skip_nl();
        while (!at_op("}")) {
            if (at(TokType::Eof)) {
                throw KestralError("Map { was never closed with }.",
                                   open.line, open.col, "syntax", true);
            }
            MapExpr::Entry entry;
            if (at(TokType::Str)) {
                entry.literal_key = advance().sval;
            } else if (at(TokType::Num)) {
                entry.literal_key = tidy_number(advance().dval);
            } else if (at_id_or_kw()) {
                entry.literal_key = advance().sval;
            } else if (at_op("(")) {
                advance();
                entry.has_expr_key = true;
                entry.expr_key = parse_expr();
                expect_op(")", "expected ) after map key");
            } else {
                throw KestralError("expected map key", peek().line, peek().col, "syntax");
            }
            expect_op(":", "expected \":\" after map key");
            skip_nl();
            entry.value = parse_expr();
            mp->entries.push_back(std::move(entry));
            skip_nl();
            if (at_op(",")) {
                advance();
                skip_nl();
            } else {
                break;
            }
        }
        expect_op("}", "expected } to close map");
        return mp;
    }

    ExprPtr parse_if_expr() {
        const Token& t = advance();
        auto node = std::make_shared<IfExpr>();
        node->line = t.line;
        node->col = t.col;
        IfBranch first;
        first.cond = parse_expr();
        skip_nl();
        first.body = parse_block();
        node->branches.push_back(std::move(first));
        for (;;) {
            std::size_t saved = pos_;
            skip_nl();
            if (!at_kw("else")) {
                pos_ = saved;
                break;
            }
            advance();
            skip_nl();
            if (at_kw("if")) {
                advance();
                IfBranch br;
                br.cond = parse_expr();
                skip_nl();
                br.body = parse_block();
                node->branches.push_back(std::move(br));
            } else {
                node->else_body = parse_block();
                break;
            }
        }
        return node;
    }

    ExprPtr parse_match_expr() {
        const Token& t = advance();
        auto node = std::make_shared<MatchExpr>();
        node->line = t.line;
        node->col = t.col;
        node->subject = parse_expr();
        skip_nl();
        const Token& open = peek();
        expect_op("{", "expected { to start match");
        skip_nl();
        while (!at_op("}")) {
            if (at(TokType::Eof)) {
                throw KestralError("This match { was never closed with }.",
                                   open.line, open.col, "syntax", true);
            }
            MatchArm arm;
            if (at_kw("else")) {
                advance();
                arm.is_else = true;
            } else {
                arm.patterns.push_back(parse_expr());
                while (at_op(",")) {
                    advance();
                    skip_nl();
                    arm.patterns.push_back(parse_expr());
                }
            }
            expect_op("=>", "expected \"=>\" in match arm");
            skip_nl();
            if (at_op("{")) {
                arm.body = parse_block();
            } else if (at_kw("say") || at_kw("give") || at_kw("fail") ||
                       at_kw("stop") || at_kw("skip") || at_kw("let")) {
                arm.body.push_back(parse_stmt());
            } else {
                arm.expr_only = true;
                arm.expr_value = parse_expr();
            }
            node->arms.push_back(std::move(arm));
            if (at(TokType::Nl)) {
                skip_nl();
            } else if (!at_op("}")) {
                throw KestralError("Put each match arm on its own line.",
                                   peek().line, peek().col, "syntax");
            }
        }
        advance(); // }
        return node;
    }
};

} // namespace

Program parse(const std::string& src) {
    auto toks = lex(src);
    Parser p(toks);
    return p.parse_program();
}

ExprPtr parse_single_expr_from_tokens(const std::vector<Token>& tokens) {
    Parser p(tokens);
    return p.parse_expression_only();
}

} // namespace kestral
