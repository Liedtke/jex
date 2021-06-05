#pragma once

#include <jex_astvisitor.hpp>
#include <jex_location.hpp>

#include <cstdint>

namespace jex {

enum class Type {
    Unresolved,
    Integer,
    Float,
    Bool,
    String
};

enum class OpType {
    Add,
    Mul
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

    AstBinaryExpr(const Location& loc, IAstExpression* lhs, IAstExpression* rhs)
    : IAstExpression(loc)
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

class AstFctCall : public IAstExpression {
public:
    AstFctCall(const Location& loc)
    : IAstExpression(loc) {
    }

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
    }
};

} // namespace jex
