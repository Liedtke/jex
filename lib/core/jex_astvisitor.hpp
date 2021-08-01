#pragma once

namespace jex {

class IAstNode;
class AstBinaryExpr;
class AstLiteralExpr;
class AstFctCall;
class AstIdentifier;
class AstArgList;

class IAstVisitor {
public:
    virtual ~IAstVisitor() {}

    virtual void visit(AstLiteralExpr& node) = 0;
    virtual void visit(AstBinaryExpr& node) = 0;
    virtual void visit(AstFctCall& node) = 0;
    virtual void visit(AstIdentifier& node) = 0;
    virtual void visit(AstArgList& node) = 0;
};

} // namespace jex
