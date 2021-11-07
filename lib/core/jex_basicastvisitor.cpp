#include <jex_basicastvisitor.hpp>

#include <jex_ast.hpp>

namespace jex {

void BasicAstVisitor::visit(AstLiteralExpr& node) {
}

void BasicAstVisitor::visit(AstBinaryExpr& node) {
    node.d_lhs->accept(*this);
    node.d_rhs->accept(*this);
}

void BasicAstVisitor::visit(AstLogicalBinExpr& node) {
    node.d_lhs->accept(*this);
    node.d_rhs->accept(*this);
}

void BasicAstVisitor::visit(AstUnaryExpr& node) {
    node.d_expr->accept(*this);
}

void BasicAstVisitor::visit(AstFctCall& node) {
    node.d_fct->accept(*this);
    node.d_args->accept(*this);
}

void BasicAstVisitor::visit(AstIf& node) {
    // Use function call implementation by default.
    BasicAstVisitor::visit(static_cast<AstFctCall&>(node));
}

void BasicAstVisitor::visit(AstIdentifier& node) {
}

void BasicAstVisitor::visit(AstArgList& node) {
    for (IAstExpression* expr : node.d_args) {
        expr->accept(*this);
    }
}

void BasicAstVisitor::visit(AstVarArg& node) {
    for (IAstExpression* expr : node.d_args) {
        expr->accept(*this);
    }
}

void BasicAstVisitor::visit(AstVariableDef& node) {
    node.d_name->accept(*this);
    node.d_type->accept(*this);
    node.d_expr->accept(*this);
}

void BasicAstVisitor::visit(AstConstantExpr& node) {
}

void BasicAstVisitor::visit(AstRoot& node) {
    for (AstVariableDef* def : node.d_varDefs) {
        def->accept(*this);
    }
}

} // namespace jex
