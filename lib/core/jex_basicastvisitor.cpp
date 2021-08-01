#include <jex_basicastvisitor.hpp>

#include <jex_ast.hpp>

namespace jex {

void BasicAstVisitor::visit(AstLiteralExpr& node) {
}

void BasicAstVisitor::visit(AstBinaryExpr& node) {
    node.d_lhs->accept(*this);
    node.d_rhs->accept(*this);
}

void BasicAstVisitor::visit(AstFctCall& node) {
    node.d_fct->accept(*this);
    node.d_args->accept(*this);
}

void BasicAstVisitor::visit(AstIdentifier& node) {
}

void BasicAstVisitor::visit(AstArgList& node) {
    for (IAstExpression* expr : node.d_args) {
        expr->accept(*this);
    }
}

} // namespace jex
