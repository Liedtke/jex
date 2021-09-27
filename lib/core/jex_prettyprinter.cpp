#include <jex_prettyprinter.hpp>

#include <jex_ast.hpp>
#include <jex_errorhandling.hpp>

#include <iostream>

namespace jex {

void PrettyPrinter::visit(AstLiteralExpr& node) {
    switch (node.d_resultType->kind()) {
        case TypeId::Bool:
            d_str << (node.d_value.d_bool ? "true" : "false");
            break;
        case TypeId::Float:
            d_str << node.d_value.d_float;
            break;
        case TypeId::Integer:
            d_str << node.d_value.d_int;
            break;
        case TypeId::String:
            // TODO: Should the pretty printer escape the string again?
            d_str << '"' << node.d_value.d_str << '"';
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
        case OpType::EQ:
            d_str << " == ";
            break;
        case OpType::NE:
            d_str << " != ";
            break;
        case OpType::LT:
            d_str << " < ";
            break;
        case OpType::GT:
            d_str << " > ";
            break;
        case OpType::LE:
            d_str << " <= ";
            break;
        case OpType::GE:
            d_str << " >= ";
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

void PrettyPrinter::visit(AstIf& node) {
    PrettyPrinter::visit(static_cast<AstFctCall&>(node));
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

void PrettyPrinter::visit(AstVariableDef& node) {
    d_str << "var ";
    node.d_name->accept(*this);
    d_str << ": ";
    node.d_type->accept(*this);
    d_str << " = ";
    node.d_expr->accept(*this);
    d_str << ";\n";
}

void PrettyPrinter::visit(AstRoot& node) {
    for (AstVariableDef* def : node.d_varDefs) {
        def->accept(*this);
    }
}

} // namespace jex
