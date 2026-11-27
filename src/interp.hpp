#pragma once

#include <functional>
#include <string>
#include <vector>

#include "ast.hpp"
#include "value.hpp"

namespace kestral {

struct InterpConfig {
    long long max_steps = 5000000000LL;
    int max_depth = 200;
    std::size_t max_output = 2000000;
    std::function<void(const std::string&)> print_fn;
    std::function<Value(const std::string&)> ask_fn;
    std::function<std::string(const std::string&)> read_file_fn;
    std::vector<std::string> cli_args;
};

struct RunOutcome {
    bool ok = true;
    std::string text;
    Value last_value;
    std::string error_text;
    std::string error_msg;
    int error_line = 0;
    bool error_fatal = false;
};

struct Interp {
    InterpConfig cfg;
    std::shared_ptr<Env> global_env;
    std::vector<std::string> builtin_names;
    long long steps = 0;
    int depth = 0;
    std::size_t printed_bytes = 0;
    bool last_was_expr = false;

    explicit Interp(InterpConfig config = {});

    Value run(const std::string& src);
    void exec_block(const std::vector<StmtPtr>& body, std::shared_ptr<Env> env,
                    bool hoist = true);
    Value eval_block_expr(const std::vector<StmtPtr>& body, std::shared_ptr<Env> env);
    Value exec_stmt(const StmtPtr& stmt, std::shared_ptr<Env> env, bool* was_expr = nullptr);
    Value eval_expr(const ExprPtr& expr, std::shared_ptr<Env> env);
    Value call_value(const Value& callee, const std::vector<Value>& args,
                     int line, int col);
    void emit_print(const std::string& line_str, int line, int col);
    void step_tick(int line, int col);
    std::vector<std::string> defined_var_names() const;
};

RunOutcome run_captured(const std::string& src,
                        const std::vector<std::string>& inputs = {},
                        const InterpConfig& base_cfg = {});

} // namespace kestral
