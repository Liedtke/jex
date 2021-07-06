#include <jex_prettyprinter.hpp>

#include <jex_ast.hpp>
#include <jex_errorhandling.hpp>

#include <iostream>

namespace jex {

void PrettyPrinter::visit(AstLiteralExpr& node) {
    switch (node.d_resultType) {
        case Type::Float:
            d_str << node.d_value.d_float;
            break;
        case Type::Integer:
            d_str << node.d_value.d_int;
            break;
        default:
            throw CompileError::create(node.d_loc, "Literal type unsupported by PrettyPrinter");
    }
}

void PrettyPrinter::visit(AstBinaryExpr& node) {
    d_str << "(";
    node.d_lhs->accept(*this);
    switch(node.d_op) {
        case OpType::Add:
            d_str << " + ";
            break;
        case OpType::Sub:
            d_str << " - ";
            break;
        case OpType::Mul:
            d_str << " * ";
            break;
        case OpType::Div:
            d_str << " / ";
            break;
        case OpType::Mod:
            d_str << " % ";
            break;
    }
    node.d_rhs->accept(*this);
    d_str << ")";
}

void PrettyPrinter::visit(AstFctCall& node) {
    node.d_fct->accept(*this);
    d_str << "(";
    node.d_args->accept(*this);
    d_str << ")";
}

void PrettyPrinter::visit(AstIdentifier& node) {
    d_str << node.d_name;
}

void PrettyPrinter::visit(AstArgList& node) {
    bool first = true;
    for (IAstExpression* expr : node.d_args) {
        if (first) {
            first = false;
        } else {
            d_str << ", ";
        }
        expr->accept(*this);
    }
}

} // namespace jex
