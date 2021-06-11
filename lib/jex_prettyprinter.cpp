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
        case OpType::Mul:
            d_str << " * ";
            break;
    }
    node.d_rhs->accept(*this);
    d_str << ")";
}

void PrettyPrinter::visit(AstFctCall& node) {
    // FIXME: implement
}

} // namespace jex
