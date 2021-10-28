#pragma once

#include <jex_astvisitor.hpp>
#include <jex_base.hpp>
#include <jex_location.hpp>
#include <jex_typeinfo.hpp>

#include <cstdint>
#include <string_view>
#include <variant>
#include <vector>

namespace jex {

class FctInfo;
struct Symbol;

enum class OpType {
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    EQ,
    NE,
    LT,
    GT,
    LE,
    GE
};

class IAstNode : NoCopy {
public:
    Location d_loc;

    IAstNode(const Location& loc)
    : d_loc(loc) {
    }
    virtual ~IAstNode() = default;

    virtual void accept(IAstVisitor& visitor) = 0;
};

class IAstExpression : public IAstNode {
public:
    TypeInfoId d_resultType;

    IAstExpression(const Location& loc, TypeInfoId type)
    : IAstNode(loc)
    , d_resultType(type) {
    }
    virtual bool isTemporary() const = 0;
    virtual bool isConstant() const {
        return false;
    }
};

class AstConstantExpr : public IAstExpression {
public:
    std::string d_constantName;

    AstConstantExpr(IAstExpression& replaced);

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    bool isConstant() const override {
        return true;
    }

    bool isTemporary() const override {
        return false;
    }
};

class AstBinaryExpr : public IAstExpression {
public:
    OpType d_op;
    IAstExpression* d_lhs;
    IAstExpression* d_rhs;
    const FctInfo* d_fctInfo = nullptr;

    AstBinaryExpr(const Location& loc, TypeInfoId resultType, OpType op, IAstExpression* lhs, IAstExpression* rhs)
    : IAstExpression(loc, resultType)
    , d_op(op)
    , d_lhs(lhs)
    , d_rhs(rhs) {
    }

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    bool isTemporary() const override {
        // Operators are handled via function calls and every function has to return a temporary.
        return true;
    }
};

class AstLiteralExpr : public IAstExpression {
public:
    std::variant<double, int64_t, bool, std::string_view> d_value;

    AstLiteralExpr(const Location& loc, TypeInfoId type, int64_t value)
    : IAstExpression(loc, type)
    , d_value(value) {
    }

    AstLiteralExpr(const Location& loc, TypeInfoId type, double value)
    : IAstExpression(loc, type)
    , d_value(value) {
    }

    AstLiteralExpr(const Location& loc, TypeInfoId type, std::string_view value)
    : IAstExpression(loc, type)
    , d_value(value) {
    }

    AstLiteralExpr(const Location& loc, TypeInfoId type, bool value)
    : IAstExpression(loc, type)
    , d_value(value) {
    }

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    bool isTemporary() const override {
        return false;
    }

    bool isConstant() const override {
        return true;
    }
};

class AstIdentifier : public IAstExpression  {
public:
    std::string_view d_name;
    Symbol *d_symbol = nullptr;

    AstIdentifier(const Location& loc, TypeInfoId type, std::string_view name)
    : IAstExpression(loc, type)
    , d_name(name) {
    }

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    bool isTemporary() const override {
        return false;
    }
};

class AstArgList : public IAstNode {
public:
    std::vector<IAstExpression*> d_args;

    AstArgList(const Location& loc)
    : IAstNode(loc) {
    }

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    void addArg(IAstExpression* arg);
};

class AstFctCall : public IAstExpression {
public:
    AstIdentifier* d_fct;
    AstArgList* d_args;
    const FctInfo* d_fctInfo = nullptr;

    AstFctCall(const Location& loc, TypeInfoId resultType, AstIdentifier* fct, AstArgList* args)
    : IAstExpression(loc, resultType)
    , d_fct(fct)
    , d_args(args) {
    }

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    bool isTemporary() const override {
        // Every function has to return a temporary.
        return true;
    }
};

class AstIf : public AstFctCall {
public:
    AstIf(const Location& loc, TypeInfoId resultType, AstIdentifier* fct, AstArgList* args)
    : AstFctCall(loc, resultType, fct, args) {
    }

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    bool isTemporary() const override {
        // An if() isn't a regular function, it doesn't return but just takes one of two values.
        // Returns a temporary if both of the values are temporaries.
        return d_args->d_args.size() == 3 && d_args->d_args[1]->isTemporary() && d_args->d_args[2]->isTemporary();
    }
};

class AstVariableDef : public IAstExpression {
public:
    AstIdentifier* d_name;
    AstIdentifier* d_type;
    IAstExpression* d_expr;

    AstVariableDef(const Location& loc, AstIdentifier* name, AstIdentifier* type, IAstExpression* expr)
    : IAstExpression(loc, type->d_resultType)
    , d_name(name)
    , d_type(type)
    , d_expr(expr) {
    }

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }

    bool isTemporary() const override {
        return false;
    }
};

class AstRoot : public IAstNode {
public:
    std::vector<AstVariableDef*> d_varDefs;

    AstRoot(const Location& loc)
    : IAstNode(loc) {
    }

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }
};

} // namespace jex
