#pragma once

namespace jex {

class IAstNode;
class IAstExpression;
class AstBinaryExpr;
class AstLogicalBinExpr;
class AstLiteralExpr;
class AstFctCall;
class AstIf;
class AstIdentifier;
class AstArgList;
class AstVariableDef;
class AstConstantExpr;
class AstUnaryExpr;
class AstVarArg;
class AstRoot;

class IAstVisitor {
public:
    virtual ~IAstVisitor() = default;

    virtual void visit(AstLiteralExpr& node) = 0;
    virtual void visit(AstBinaryExpr& node) = 0;
    virtual void visit(AstLogicalBinExpr& node) = 0;
    virtual void visit(AstUnaryExpr& node) = 0;
    virtual void visit(AstFctCall& node) = 0;
    virtual void visit(AstIf& node) = 0;
    virtual void visit(AstIdentifier& node) = 0;
    virtual void visit(AstArgList& node) = 0;
    virtual void visit(AstConstantExpr& node) = 0;
    virtual void visit(AstVariableDef& node) = 0;
    virtual void visit(AstVarArg& node) = 0;
    virtual void visit(AstRoot& node) = 0;
};

} // namespace jex
