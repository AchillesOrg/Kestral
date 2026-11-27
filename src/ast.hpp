#pragma once

#include <memory>
#include <string>
#include <vector>

namespace kestral {

struct Expr {
    int line = 0;
    int col = 0;
    virtual ~Expr() = default;
};

struct Stmt {
    int line = 0;
    int col = 0;
    virtual ~Stmt() = default;
};

using ExprPtr = std::shared_ptr<Expr>;
using StmtPtr = std::shared_ptr<Stmt>;

struct LiteralExpr : Expr {
    enum class Kind { Nothing, Bool, Number, Text };
    Kind kind = Kind::Nothing;
    bool bval = false;
    double dval = 0.0;
    std::string sval;
};

struct InterpTextExpr : Expr {
    struct Segment {
        bool is_expr = false;
        std::string text;
        ExprPtr expr;
    };
    std::vector<Segment> segments;
};

struct IdentExpr : Expr {
    std::string name;
};

struct ListExpr : Expr {
    std::vector<ExprPtr> elements;
};

struct MapExpr : Expr {
    struct Entry {
        std::string literal_key;
        bool has_expr_key = false;
        ExprPtr expr_key;
        ExprPtr value;
    };
    std::vector<Entry> entries;
};

struct UnaryExpr : Expr {
    std::string op; // "-" | "not"
    ExprPtr operand;
};

struct BinaryExpr : Expr {
    std::string op;
    ExprPtr left;
    ExprPtr right;
};

struct CallExpr : Expr {
    ExprPtr callee;
    std::vector<ExprPtr> args;
};

struct MemberExpr : Expr {
    ExprPtr object;
    std::string member;
    int member_line = 0;
    int member_col = 0;
};

struct IndexExpr : Expr {
    ExprPtr object;
    ExprPtr index;
};

struct PipeExpr : Expr {
    ExprPtr value;
    std::string target_name;
    ExprPtr target_expr;
    std::vector<ExprPtr> extra_args;
    int target_line = 0;
    int target_col = 0;
};

struct LambdaExpr : Expr {
    std::string name;
    std::vector<std::string> param_names;
    std::vector<ExprPtr> param_defaults;
    std::vector<StmtPtr> body;
};

struct IfBranch {
    ExprPtr cond;
    std::vector<StmtPtr> body;
};

struct IfExpr : Expr {
    std::vector<IfBranch> branches;
    std::vector<StmtPtr> else_body;
};

struct MatchArm {
    bool is_else = false;
    std::vector<ExprPtr> patterns;
    std::vector<StmtPtr> body;
    bool expr_only = false;
    ExprPtr expr_value;
};

struct MatchExpr : Expr {
    ExprPtr subject;
    std::vector<MatchArm> arms;
};

struct LetStmt : Stmt {
    std::string name;
    ExprPtr value;
};

struct AssignStmt : Stmt {
    ExprPtr target; // IdentExpr | MemberExpr | IndexExpr
    std::string op; // "=" | "+=" | "-=" | "*=" | "/=" | "%="
    ExprPtr value;
};

struct FnStmt : Stmt {
    std::string name;
    std::vector<std::string> param_names;
    std::vector<ExprPtr> param_defaults;
    std::vector<StmtPtr> body;
};

struct SayStmt : Stmt {
    std::vector<ExprPtr> args;
};

struct GiveStmt : Stmt {
    ExprPtr value; // optional
};

struct StopStmt : Stmt {};
struct SkipStmt : Stmt {};

struct WhileStmt : Stmt {
    ExprPtr cond;
    std::vector<StmtPtr> body;
};

struct LoopStmt : Stmt {
    ExprPtr count;
    std::vector<StmtPtr> body;
};

struct ForStmt : Stmt {
    std::string var1;
    std::string var2; // empty unless `for i, x in ...`
    ExprPtr iterable;
    std::vector<StmtPtr> body;
};

struct TryStmt : Stmt {
    std::vector<StmtPtr> try_body;
    std::string err_var;
    std::vector<StmtPtr> catch_body;
};

struct FailStmt : Stmt {
    ExprPtr value;
};

struct UseStmt : Stmt {
    ExprPtr path;
};

struct ExprStmt : Stmt {
    ExprPtr expr;
};

struct Program {
    std::vector<StmtPtr> body;
};

} // namespace kestral
