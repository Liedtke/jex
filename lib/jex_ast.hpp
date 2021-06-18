#pragma once

#include <jex_astvisitor.hpp>
#include <jex_location.hpp>
#include <jex_typesystem.hpp>

#include <cstdint>
#include <string_view>
#include <vector>

namespace jex {

struct Symbol;

enum class OpType {
    Add,
    Sub,
    Mul,
    Div,
    Mod
};

class IAstNode {
public:
    Location d_loc;

    IAstNode(const Location& loc)
    : d_loc(loc) {
    }
    virtual ~IAstNode() {}

    virtual void accept(IAstVisitor& visitor) = 0;
};

class IAstExpression : public IAstNode {
public:
    Type d_resultType;

    IAstExpression(const Location& loc, Type type = Type::Unresolved)
    : IAstNode(loc)
    , d_resultType(type) {
    }
};

class AstBinaryExpr : public IAstExpression {
public:
    OpType d_op;
    IAstExpression* d_lhs;
    IAstExpression* d_rhs;

    AstBinaryExpr(const Location& loc, OpType op, IAstExpression* lhs, IAstExpression* rhs)
    : IAstExpression(loc)
    , d_op(op)
    , d_lhs(lhs)
    , d_rhs(rhs) {
    }

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }
};

class AstLiteralExpr : public IAstExpression {
public:
    union Value {
        double d_float;
        int64_t d_int;
    } d_value;

    AstLiteralExpr(const Location& loc)
    : IAstExpression(loc) {
    }

    AstLiteralExpr(const Location& loc, int64_t value)
    : IAstExpression(loc, Type::Integer) {
        d_value.d_int = value;
    }

    AstLiteralExpr(const Location& loc, double value)
    : IAstExpression(loc, Type::Float) {
        d_value.d_float = value;
    }

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }
};

class AstIdentifier : public IAstExpression  {
public:
    std::string_view d_name;
    Symbol *d_symbol = nullptr;

    AstIdentifier(const Location& loc, std::string_view name)
    : IAstExpression(loc)
    , d_name(name) {
    }

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
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

    AstFctCall(const Location& loc, AstIdentifier* fct, AstArgList* args)
    : IAstExpression(loc)
    , d_fct(fct)
    , d_args(args) {
    }

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }
};

} // namespace jex
