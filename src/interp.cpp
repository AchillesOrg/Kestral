#include "interp.hpp"
#include "builtins.hpp"
#include "lexer.hpp"
#include "parser.hpp"

#include <cmath>
#include <iostream>
#include <sstream>
#include <unordered_set>

namespace kestral {

static std::string make_missing_member_msg(const Value& obj, const std::string& mem) {
    if (obj.is_nothing()) return "Cannot read \"" + mem + "\" because the value is nothing.";
    return type_name(obj) + " has no \"" + mem + "\".";
}

Interp::Interp(InterpConfig config) : cfg(std::move(config)) {
    global_env = std::make_shared<Env>(nullptr);
    register_builtins(*this);
}

void Interp::step_tick(int line, int col) {
    if (++steps > cfg.max_steps) {
        throw KestralError("This program ran for too long (maybe an infinite loop).",
                           line, col, "runtime", false, true);
    }
}

void Interp::emit_print(const std::string& line_str, int line, int col) {
    printed_bytes += line_str.size() + 1;
    if (printed_bytes > cfg.max_output) {
        throw KestralError("This program printed too much output.",
                           line, col, "runtime", false, true);
    }
    if (cfg.print_fn) {
        cfg.print_fn(line_str);
    } else {
        std::cout << line_str << '\n';
    }
}

std::vector<std::string> Interp::defined_var_names() const {
    std::vector<std::string> out;
    std::unordered_set<std::string> bset(builtin_names.begin(), builtin_names.end());
    for (const auto& name : global_env->insertion_order) {
        if (bset.find(name) == bset.end()) out.push_back(name);
    }
    return out;
}

Value Interp::run(const std::string& src) {
    Program prog = parse(src);
    // Hoist functions
    for (const auto& stmt : prog.body) {
        if (auto* fn = dynamic_cast<FnStmt*>(stmt.get())) {
            auto fo = std::make_shared<FunctionObj>();
            fo->name = fn->name;
            fo->param_names = fn->param_names;
            fo->param_defaults = fn->param_defaults;
            fo->body = fn->body;
            fo->closure = global_env;
            global_env->define(fn->name, Value(fo));
        }
    }
    Value last;
    last_was_expr = false;
    for (const auto& stmt : prog.body) {
        bool was_e = false;
        try {
            last = exec_stmt(stmt, global_env, &was_e);
        } catch (const ReturnSig&) {
            throw KestralError("\"give\" only works inside a function.", stmt->line, stmt->col);
        } catch (const StopSig&) {
            throw KestralError("\"stop\" only works inside a loop.", stmt->line, stmt->col);
        } catch (const SkipSig&) {
            throw KestralError("\"skip\" only works inside a loop.", stmt->line, stmt->col);
        }
        last_was_expr = was_e;
    }
    return last;
}

void Interp::exec_block(const std::vector<StmtPtr>& body, std::shared_ptr<Env> env,
                        bool hoist) {
    if (hoist) {
        for (const auto& stmt : body) {
            if (auto* fn = dynamic_cast<FnStmt*>(stmt.get())) {
                auto fo = std::make_shared<FunctionObj>();
                fo->name = fn->name;
                fo->param_names = fn->param_names;
                fo->param_defaults = fn->param_defaults;
                fo->body = fn->body;
                fo->closure = env;
                env->define(fn->name, Value(fo));
            }
        }
    }
    for (const auto& stmt : body) {
        exec_stmt(stmt, env);
    }
}

Value Interp::eval_block_expr(const std::vector<StmtPtr>& body, std::shared_ptr<Env> env) {
    auto local = std::make_shared<Env>(env);
    for (const auto& stmt : body) {
        if (auto* fn = dynamic_cast<FnStmt*>(stmt.get())) {
            auto fo = std::make_shared<FunctionObj>();
            fo->name = fn->name;
            fo->param_names = fn->param_names;
            fo->param_defaults = fn->param_defaults;
            fo->body = fn->body;
            fo->closure = local;
            local->define(fn->name, Value(fo));
        }
    }
    Value last;
    for (const auto& stmt : body) {
        last = exec_stmt(stmt, local);
    }
    return last;
}

static Value apply_compound_op(const std::string& op, const Value& cur, const Value& rhs,
                               int line, int col) {
    if (op == "=") return rhs;
    if (op == "+=") {
        if (cur.is_text() || rhs.is_text()) return Value(format_value(cur) + format_value(rhs));
        if (cur.is_list() && rhs.is_list()) {
            auto out = std::make_shared<ListObj>();
            out->items = cur.as_list()->items;
            out->items.insert(out->items.end(), rhs.as_list()->items.begin(), rhs.as_list()->items.end());
            return Value(out);
        }
        if (cur.is_number() && rhs.is_number()) return Value(cur.as_number() + rhs.as_number());
        throw KestralError("I can't add " + type_name(cur) + " and " + type_name(rhs) + ".", line, col);
    }
    if (!cur.is_number() || !rhs.is_number()) {
        throw KestralError("\"" + op + "\" needs numbers.", line, col);
    }
    double a = cur.as_number(), b = rhs.as_number();
    if (op == "-=") return Value(a - b);
    if (op == "*=") return Value(a * b);
    if (op == "/=") {
        if (b == 0.0) throw KestralError("Cannot divide by zero.", line, col);
        return Value(a / b);
    }
    if (op == "%=") {
        if (b == 0.0) throw KestralError("Cannot divide by zero.", line, col);
        return Value(floored_mod(a, b));
    }
    return rhs;
}

Value Interp::exec_stmt(const StmtPtr& stmt, std::shared_ptr<Env> env, bool* was_expr) {
    if (was_expr) *was_expr = false;
    step_tick(stmt->line, stmt->col);

    if (auto* s = dynamic_cast<LetStmt*>(stmt.get())) {
        Value v = eval_expr(s->value, env);
        env->define(s->name, std::move(v));
        return Value();
    }

    if (auto* s = dynamic_cast<FnStmt*>(stmt.get())) {
        // Already hoisted when block starts; refresh definition cleanly in REPL.
        auto fo = std::make_shared<FunctionObj>();
        fo->name = s->name;
        fo->param_names = s->param_names;
        fo->param_defaults = s->param_defaults;
        fo->body = s->body;
        fo->closure = env;
        env->define(s->name, Value(fo));
        return Value();
    }

    if (auto* s = dynamic_cast<AssignStmt*>(stmt.get())) {
        Value rhs = eval_expr(s->value, env);
        if (auto* id = dynamic_cast<IdentExpr*>(s->target.get())) {
            Value new_val = rhs;
            if (s->op != "=") {
                Value cur = env->get(id->name, id->line, id->col, &builtin_names);
                new_val = apply_compound_op(s->op, cur, rhs, s->line, s->col);
            }
            env->assign(id->name, std::move(new_val), id->line, id->col);
            return Value();
        }
        if (auto* ix = dynamic_cast<IndexExpr*>(s->target.get())) {
            Value obj = eval_expr(ix->object, env);
            Value key = eval_expr(ix->index, env);
            if (obj.is_text()) {
                throw KestralError("Text can't be changed in place.", ix->line, ix->col);
            }
            if (obj.is_list()) {
                if (!key.is_number() || !is_whole_number(key.as_number())) {
                    throw KestralError("List positions must be whole numbers.", ix->line, ix->col);
                }
                auto& items = obj.as_list()->items;
                long long idx = static_cast<long long>(key.as_number());
                long long len = static_cast<long long>(items.size());
                if (idx < 0) idx += len;
                if (idx < 0 || idx >= len) {
                    throw KestralError("Position " + std::to_string(static_cast<long long>(key.as_number())) +
                                           " is outside this list (length " + std::to_string(len) + ").",
                                       ix->line, ix->col);
                }
                Value new_val = rhs;
                if (s->op != "=") {
                    new_val = apply_compound_op(s->op, items[static_cast<std::size_t>(idx)], rhs, s->line, s->col);
                }
                items[static_cast<std::size_t>(idx)] = std::move(new_val);
                return Value();
            }
            if (obj.is_map()) {
                std::string mk = map_key_from_value(key, ix->line, ix->col);
                Value new_val = rhs;
                if (s->op != "=") {
                    new_val = apply_compound_op(s->op, obj.as_map()->get(mk), rhs, s->line, s->col);
                }
                obj.as_map()->set(mk, std::move(new_val));
                return Value();
            }
            throw KestralError("Cannot assign item on " + type_name(obj) + ".", ix->line, ix->col);
        }
        if (auto* mem = dynamic_cast<MemberExpr*>(s->target.get())) {
            Value obj = eval_expr(mem->object, env);
            if (!obj.is_map()) {
                throw KestralError("Only maps can set fields like ." + mem->member + ".", mem->line, mem->col);
            }
            Value new_val = rhs;
            if (s->op != "=") {
                new_val = apply_compound_op(s->op, obj.as_map()->get(mem->member), rhs, s->line, s->col);
            }
            obj.as_map()->set(mem->member, std::move(new_val));
            return Value();
        }
        throw KestralError("Invalid assignment target.", s->line, s->col);
    }

    if (auto* s = dynamic_cast<SayStmt*>(stmt.get())) {
        std::string line_str;
        for (std::size_t i = 0; i < s->args.size(); ++i) {
            if (i > 0) line_str += ' ';
            line_str += format_value(eval_expr(s->args[i], env), false);
        }
        emit_print(line_str, s->line, s->col);
        return Value();
    }

    if (auto* s = dynamic_cast<GiveStmt*>(stmt.get())) {
        Value v = s->value ? eval_expr(s->value, env) : Value();
        throw ReturnSig{std::move(v)};
    }
    if (dynamic_cast<StopStmt*>(stmt.get())) throw StopSig{};
    if (dynamic_cast<SkipStmt*>(stmt.get())) throw SkipSig{};

    if (auto* s = dynamic_cast<WhileStmt*>(stmt.get())) {
        while (is_truthy(eval_expr(s->cond, env))) {
            step_tick(s->line, s->col);
            auto local = std::make_shared<Env>(env);
            try {
                exec_block(s->body, local);
            } catch (const SkipSig&) {
                continue;
            } catch (const StopSig&) {
                break;
            }
        }
        return Value();
    }

    if (auto* s = dynamic_cast<LoopStmt*>(stmt.get())) {
        Value cv = eval_expr(s->count, env);
        if (!cv.is_number() || !is_whole_number(cv.as_number()) || cv.as_number() < 0) {
            throw KestralError("loop count must be a non-negative whole number.", s->line, s->col);
        }
        long long n = static_cast<long long>(cv.as_number());
        for (long long i = 0; i < n; ++i) {
            step_tick(s->line, s->col);
            auto local = std::make_shared<Env>(env);
            try {
                exec_block(s->body, local);
            } catch (const SkipSig&) {
                continue;
            } catch (const StopSig&) {
                break;
            }
        }
        return Value();
    }

    if (auto* s = dynamic_cast<ForStmt*>(stmt.get())) {
        Value it = eval_expr(s->iterable, env);
        auto run_iter = [&](Value k, Value v, bool two) -> bool {
            step_tick(s->line, s->col);
            auto local = std::make_shared<Env>(env);
            if (two && !s->var2.empty()) {
                local->define(s->var1, std::move(k));
                local->define(s->var2, std::move(v));
            } else {
                local->define(s->var1, std::move(v));
            }
            try {
                exec_block(s->body, local);
            } catch (const SkipSig&) {
                return true;
            } catch (const StopSig&) {
                return false;
            }
            return true;
        };

        if (it.is_range()) {
            const auto& r = it.as_range();
            long long step = (r.start <= r.end) ? 1 : -1;
            long long len = r.size();
            for (long long idx = 0; idx < len; ++idx) {
                long long val = r.start + idx * step;
                if (!run_iter(Value(idx), Value(val), !s->var2.empty())) break;
            }
            return Value();
        }
        if (it.is_list()) {
            auto xs = it.as_list()->items;
            for (std::size_t idx = 0; idx < xs.size(); ++idx) {
                if (!run_iter(Value(static_cast<long long>(idx)), xs[idx], !s->var2.empty())) break;
            }
            return Value();
        }
        if (it.is_text()) {
            const std::string& txt = it.as_text();
            for (std::size_t idx = 0; idx < txt.size(); ++idx) {
                if (!run_iter(Value(static_cast<long long>(idx)),
                              Value(std::string(1, txt[idx])), !s->var2.empty())) {
                    break;
                }
            }
            return Value();
        }
        if (it.is_map()) {
            auto entries = it.as_map()->entries;
            for (const auto& kv : entries) {
                if (!run_iter(Value(kv.first), kv.second, !s->var2.empty())) break;
            }
            return Value();
        }
        if (it.is_number()) {
            throw KestralError("Can't loop over a number directly — try: loop " +
                                   tidy_number(it.as_number()) + " times { ... }",
                               s->line, s->col);
        }
        throw KestralError("Can't loop over " + type_name(it) + ".", s->line, s->col);
    }

    if (auto* s = dynamic_cast<TryStmt*>(stmt.get())) {
        auto try_env = std::make_shared<Env>(env);
        try {
            exec_block(s->try_body, try_env);
        } catch (const KestralError& err) {
            if (err.fatal) throw;
            auto catch_env = std::make_shared<Env>(env);
            if (!s->err_var.empty()) {
                catch_env->define(s->err_var, Value(std::string(err.what())));
            }
            exec_block(s->catch_body, catch_env);
        }
        return Value();
    }

    if (auto* s = dynamic_cast<FailStmt*>(stmt.get())) {
        Value v = eval_expr(s->value, env);
        throw KestralError(format_value(v, false), s->line, s->col);
    }

    if (auto* s = dynamic_cast<UseStmt*>(stmt.get())) {
        Value pv = eval_expr(s->path, env);
        if (!pv.is_text()) throw KestralError("use expects a file name as text.", s->line, s->col);
        if (!cfg.read_file_fn) {
            throw KestralError("Cannot load \"" + pv.as_text() + "\": file loading is disabled here.",
                               s->line, s->col);
        }
        std::string code;
        try {
            code = cfg.read_file_fn(pv.as_text());
        } catch (const std::exception& e) {
            throw KestralError("Could not load \"" + pv.as_text() + "\": " + e.what(),
                               s->line, s->col);
        }
        Program mod = parse(code);
        exec_block(mod.body, env, true);
        return Value();
    }

    if (auto* s = dynamic_cast<ExprStmt*>(stmt.get())) {
        if (was_expr) *was_expr = true;
        return eval_expr(s->expr, env);
    }

    return Value();
}

Value Interp::eval_expr(const ExprPtr& expr, std::shared_ptr<Env> env) {
    step_tick(expr->line, expr->col);

    if (auto* e = dynamic_cast<LiteralExpr*>(expr.get())) {
        switch (e->kind) {
            case LiteralExpr::Kind::Nothing: return Value();
            case LiteralExpr::Kind::Bool: return Value(e->bval);
            case LiteralExpr::Kind::Number: return Value(e->dval);
            case LiteralExpr::Kind::Text: return Value(e->sval);
        }
    }

    if (auto* e = dynamic_cast<InterpTextExpr*>(expr.get())) {
        std::string out;
        for (const auto& seg : e->segments) {
            if (!seg.is_expr) out += seg.text;
            else out += format_value(eval_expr(seg.expr, env), false);
        }
        return Value(out);
    }

    if (auto* e = dynamic_cast<IdentExpr*>(expr.get())) {
        return env->get(e->name, e->line, e->col, &builtin_names);
    }

    if (auto* e = dynamic_cast<ListExpr*>(expr.get())) {
        auto list = std::make_shared<ListObj>();
        for (const auto& el : e->elements) list->items.push_back(eval_expr(el, env));
        return Value(list);
    }

    if (auto* e = dynamic_cast<MapExpr*>(expr.get())) {
        auto mp = std::make_shared<MapObj>();
        for (const auto& ent : e->entries) {
            std::string k = ent.has_expr_key
                                ? map_key_from_value(eval_expr(ent.expr_key, env), e->line, e->col)
                                : ent.literal_key;
            mp->set(k, eval_expr(ent.value, env));
        }
        return Value(mp);
    }

    if (auto* e = dynamic_cast<LambdaExpr*>(expr.get())) {
        auto fo = std::make_shared<FunctionObj>();
        fo->name = e->name;
        fo->param_names = e->param_names;
        fo->param_defaults = e->param_defaults;
        fo->body = e->body;
        fo->closure = env;
        return Value(fo);
    }

    if (auto* e = dynamic_cast<UnaryExpr*>(expr.get())) {
        Value v = eval_expr(e->operand, env);
        if (e->op == "not") return Value(!is_truthy(v));
        if (e->op == "-") {
            if (!v.is_number()) {
                throw KestralError("I can only negate a number, not " + type_name(v) + ".",
                                   e->line, e->col);
            }
            return Value(-v.as_number());
        }
    }

    if (auto* e = dynamic_cast<BinaryExpr*>(expr.get())) {
        if (e->op == "and") {
            Value l = eval_expr(e->left, env);
            return is_truthy(l) ? eval_expr(e->right, env) : l;
        }
        if (e->op == "or") {
            Value l = eval_expr(e->left, env);
            return is_truthy(l) ? l : eval_expr(e->right, env);
        }
        if (e->op == "??") {
            Value l = eval_expr(e->left, env);
            return l.is_nothing() ? eval_expr(e->right, env) : l;
        }

        Value l = eval_expr(e->left, env);
        Value r = eval_expr(e->right, env);

        if (e->op == "==") return Value(deep_equals(l, r));
        if (e->op == "!=") return Value(!deep_equals(l, r));

        if (e->op == "..") {
            if (!l.is_number() || !r.is_number() ||
                !is_whole_number(l.as_number()) || !is_whole_number(r.as_number())) {
                throw KestralError("Ranges need whole numbers like 1..10.", e->line, e->col);
            }
            return Value(RangeVal{static_cast<long long>(l.as_number()),
                                  static_cast<long long>(r.as_number())});
        }

        if (e->op == "+") {
            if (l.is_text() || r.is_text()) {
                return Value(format_value(l, false) + format_value(r, false));
            }
            if (l.is_list() && r.is_list()) {
                auto out = std::make_shared<ListObj>();
                out->items = l.as_list()->items;
                out->items.insert(out->items.end(), r.as_list()->items.begin(), r.as_list()->items.end());
                return Value(out);
            }
            if (l.is_number() && r.is_number()) return Value(l.as_number() + r.as_number());
            throw KestralError("I can't add " + type_name(l) + " and " + type_name(r) + ".",
                               e->line, e->col);
        }

        if (e->op == "*") {
            if (l.is_number() && r.is_number()) return Value(l.as_number() * r.as_number());
            auto repeat_s = [&](const std::string& s, double times) -> Value {
                if (!is_whole_number(times) || times < 0) {
                    throw KestralError("Text can only repeat a non-negative whole number of times.",
                                       e->line, e->col);
                }
                long long n = static_cast<long long>(times);
                std::string out;
                out.reserve(s.size() * static_cast<std::size_t>(n));
                for (long long i = 0; i < n; ++i) out += s;
                return Value(out);
            };
            if (l.is_text() && r.is_number()) return repeat_s(l.as_text(), r.as_number());
            if (l.is_number() && r.is_text()) return repeat_s(r.as_text(), l.as_number());
            throw KestralError("\"*\" needs numbers (or text * count).", e->line, e->col);
        }

        if (e->op == "<" || e->op == "<=" || e->op == ">" || e->op == ">=") {
            if (l.is_number() && r.is_number()) {
                double a = l.as_number(), b = r.as_number();
                if (e->op == "<") return Value(a < b);
                if (e->op == "<=") return Value(a <= b);
                if (e->op == ">") return Value(a > b);
                return Value(a >= b);
            }
            if (l.is_text() && r.is_text()) {
                if (e->op == "<") return Value(l.as_text() < r.as_text());
                if (e->op == "<=") return Value(l.as_text() <= r.as_text());
                if (e->op == ">") return Value(l.as_text() > r.as_text());
                return Value(l.as_text() >= r.as_text());
            }
            throw KestralError("I can't compare " + type_name(l) + " with " + type_name(r) + ".",
                               e->line, e->col);
        }

        if (!l.is_number() || !r.is_number()) {
            throw KestralError("\"" + e->op + "\" needs numbers.", e->line, e->col);
        }
        double a = l.as_number(), b = r.as_number();
        if (e->op == "-") return Value(a - b);
        if (e->op == "/") {
            if (b == 0.0) throw KestralError("Cannot divide by zero.", e->line, e->col);
            return Value(a / b);
        }
        if (e->op == "%") {
            if (b == 0.0) throw KestralError("Cannot divide by zero.", e->line, e->col);
            return Value(floored_mod(a, b));
        }
        if (e->op == "**") return Value(std::pow(a, b));
    }

    if (auto* e = dynamic_cast<MemberExpr*>(expr.get())) {
        Value obj = eval_expr(e->object, env);
        if (obj.is_map()) return obj.as_map()->get(e->member);
        throw KestralError(make_missing_member_msg(obj, e->member), e->member_line, e->member_col);
    }

    if (auto* e = dynamic_cast<IndexExpr*>(expr.get())) {
        Value obj = eval_expr(e->object, env);
        Value key = eval_expr(e->index, env);
        if (obj.is_map()) {
            return obj.as_map()->get(map_key_from_value(key, e->line, e->col));
        }
        if (!key.is_number() || !is_whole_number(key.as_number())) {
            throw KestralError("Positions must be whole numbers.", e->line, e->col);
        }
        long long idx = static_cast<long long>(key.as_number());
        if (obj.is_list()) {
            const auto& xs = obj.as_list()->items;
            long long len = static_cast<long long>(xs.size());
            long long real_i = (idx < 0) ? (idx + len) : idx;
            if (real_i < 0 || real_i >= len) {
                throw KestralError("Position " + std::to_string(idx) +
                                       " is outside this list (length " + std::to_string(len) + ").",
                                   e->line, e->col);
            }
            return xs[static_cast<std::size_t>(real_i)];
        }
        if (obj.is_text()) {
            const auto& s = obj.as_text();
            long long len = static_cast<long long>(s.size());
            long long real_i = (idx < 0) ? (idx + len) : idx;
            if (real_i < 0 || real_i >= len) {
                throw KestralError("Position " + std::to_string(idx) +
                                       " is outside this text (length " + std::to_string(len) + ").",
                                   e->line, e->col);
            }
            return Value(std::string(1, s[static_cast<std::size_t>(real_i)]));
        }
        if (obj.is_range()) return Value(obj.as_range().at(idx));
        throw KestralError("Cannot index into " + type_name(obj) + ".", e->line, e->col);
    }

    if (auto* e = dynamic_cast<CallExpr*>(expr.get())) {
        // Check dot method style: `obj.method(args...)`
        if (auto* mem = dynamic_cast<MemberExpr*>(e->callee.get())) {
            Value obj = eval_expr(mem->object, env);
            std::vector<Value> args;
            for (const auto& a : e->args) args.push_back(eval_expr(a, env));
            if (obj.is_map() && obj.as_map()->has(mem->member)) {
                Value method_val = obj.as_map()->get(mem->member);
                return call_value(method_val, args, e->line, e->col);
            }
            Value fn_val = env->get(mem->member, mem->member_line, mem->member_col, &builtin_names);
            std::vector<Value> full_args;
            full_args.push_back(std::move(obj));
            full_args.insert(full_args.end(), args.begin(), args.end());
            return call_value(fn_val, full_args, e->line, e->col);
        }

        Value callee = eval_expr(e->callee, env);
        std::vector<Value> args;
        for (const auto& a : e->args) args.push_back(eval_expr(a, env));
        return call_value(callee, args, e->line, e->col);
    }

    if (auto* e = dynamic_cast<PipeExpr*>(expr.get())) {
        Value lhs = eval_expr(e->value, env);
        Value target = eval_expr(e->target_expr, env);
        std::vector<Value> args;
        args.push_back(std::move(lhs));
        for (const auto& a : e->extra_args) args.push_back(eval_expr(a, env));
        return call_value(target, args, e->target_line, e->target_col);
    }

    if (auto* e = dynamic_cast<IfExpr*>(expr.get())) {
        for (const auto& br : e->branches) {
            if (is_truthy(eval_expr(br.cond, env))) {
                return eval_block_expr(br.body, env);
            }
        }
        if (!e->else_body.empty()) {
            return eval_block_expr(e->else_body, env);
        }
        return Value();
    }

    if (auto* e = dynamic_cast<MatchExpr*>(expr.get())) {
        Value subj = eval_expr(e->subject, env);
        for (const auto& arm : e->arms) {
            bool matched = arm.is_else;
            if (!matched) {
                for (const auto& pat_expr : arm.patterns) {
                    Value pat = eval_expr(pat_expr, env);
                    if (pat.is_range() && subj.is_number() && is_whole_number(subj.as_number())) {
                        if (pat.as_range().contains(static_cast<long long>(subj.as_number()))) {
                            matched = true;
                            break;
                        }
                    } else if (deep_equals(subj, pat)) {
                        matched = true;
                        break;
                    }
                }
            }
            if (matched) {
                if (arm.expr_only) return eval_expr(arm.expr_value, env);
                return eval_block_expr(arm.body, env);
            }
        }
        return Value();
    }

    return Value();
}

Value Interp::call_value(const Value& callee, const std::vector<Value>& args,
                         int line, int col) {
    if (callee.is_builtin()) {
        const auto& b = std::get<std::shared_ptr<BuiltinObj>>(callee.data);
        return b->fn(*this, args, line, col);
    }
    if (!callee.is_function()) {
        throw KestralError("You tried to call a " + type_name(callee) + " — it is not a function.",
                           line, col);
    }
    const auto& fn = std::get<std::shared_ptr<FunctionObj>>(callee.data);
    std::size_t max_params = fn->param_names.size();
    std::size_t min_params = 0;
    for (const auto& d : fn->param_defaults) {
        if (!d) min_params++;
    }
    if (args.size() < min_params || args.size() > max_params) {
        std::string fname = fn->name.empty() ? "this function" : (fn->name + "()");
        throw KestralError(fname + " needs " + std::to_string(min_params) +
                               (min_params == 1 ? " value" : " values") +
                               ", got " + std::to_string(args.size()) + ".",
                           line, col);
    }
    if (++depth > cfg.max_depth) {
        --depth;
        throw KestralError("Too many nested function calls — recursion went too deep.",
                           line, col, "runtime", false, true);
    }
    struct DepthGuard {
        int& d;
        ~DepthGuard() { d--; }
    } guard{depth};

    auto local = std::make_shared<Env>(fn->closure);
    for (std::size_t i = 0; i < max_params; ++i) {
        if (i < args.size()) {
            local->define(fn->param_names[i], args[i]);
        } else if (fn->param_defaults[i]) {
            local->define(fn->param_names[i], eval_expr(fn->param_defaults[i], local));
        } else {
            local->define(fn->param_names[i], Value());
        }
    }

    try {
        exec_block(fn->body, local, true);
    } catch (const ReturnSig& ret) {
        return ret.value;
    } catch (const StopSig&) {
        throw KestralError("\"stop\" only works inside a loop.", line, col);
    } catch (const SkipSig&) {
        throw KestralError("\"skip\" only works inside a loop.", line, col);
    }
    return Value();
}

RunOutcome run_captured(const std::string& src,
                        const std::vector<std::string>& inputs,
                        const InterpConfig& base_cfg) {
    RunOutcome res;
    std::size_t in_pos = 0;
    InterpConfig cfg = base_cfg;
    std::ostringstream captured;
    bool first = true;
    cfg.print_fn = [&](const std::string& s) {
        if (!first) captured << '\n';
        first = false;
        captured << s;
    };
    cfg.ask_fn = [&](const std::string&) -> Value {
        if (in_pos < inputs.size()) return Value(inputs[in_pos++]);
        return Value(); // nothing
    };

    try {
        Interp interp(cfg);
        res.last_value = interp.run(src);
        res.text = captured.str();
        res.ok = true;
    } catch (const KestralError& err) {
        res.ok = false;
        res.text = captured.str();
        res.error_msg = err.what();
        res.error_line = err.line;
        res.error_fatal = err.fatal;
        res.error_text = format_error(err, src);
    }
    return res;
}

} // namespace kestral
