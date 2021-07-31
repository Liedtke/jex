#pragma once

#include <jex_astvisitor.hpp>
#include <jex_base.hpp>
#include <jex_location.hpp>
#include <jex_typeinfo.hpp>

#include <cstdint>
#include <string_view>
#include <vector>

namespace jex {

class FctInfo;
struct Symbol;

enum class OpType {
    Add,
    Sub,
    Mul,
    Div,
    Mod
};

class IAstNode : NoCopy {
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
    TypeInfoId d_resultType;

    IAstExpression(const Location& loc, TypeInfoId type)
    : IAstNode(loc)
    , d_resultType(type) {
    }
};

class AstBinaryExpr : public IAstExpression {
public:
    OpType d_op;
    IAstExpression* d_lhs;
    IAstExpression* d_rhs;

    AstBinaryExpr(const Location& loc, TypeInfoId resultType, OpType op, IAstExpression* lhs, IAstExpression* rhs)
    : IAstExpression(loc, resultType)
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
        std::string_view d_str;

        Value() {}
    } d_value;

    AstLiteralExpr(const Location& loc, TypeInfoId type, int64_t value)
    : IAstExpression(loc, type) {
        d_value.d_int = value;
    }

    AstLiteralExpr(const Location& loc, TypeInfoId type, double value)
    : IAstExpression(loc, type) {
        d_value.d_float = value;
    }

    AstLiteralExpr(const Location& loc, TypeInfoId type, std::string_view value)
    : IAstExpression(loc, type) {
        new (&d_value.d_str) std::string_view(value);
    }

    void accept(IAstVisitor& visitor) override {
        visitor.visit(*this);
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
};

} // namespace jex
