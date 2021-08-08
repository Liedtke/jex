#pragma once

#include <jex_astvisitor.hpp>

namespace jex {

class BasicAstVisitor : public IAstVisitor {
public:
    void visit(AstLiteralExpr& node) override;
    void visit(AstBinaryExpr& node) override;
    void visit(AstFctCall& node) override;
    void visit(AstIdentifier& node) override;
    void visit(AstArgList& node) override;
    void visit(AstVariableDef& node) override;
    void visit(AstRoot& node) override;
};

} // namespace jex
