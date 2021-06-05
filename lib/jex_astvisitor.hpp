#pragma once

namespace jex {

class AstBinaryExpr;
class AstLiteralExpr;
class AstFctCall;

class IAstVisitor {
public:
    virtual ~IAstVisitor() {}

    virtual void visit(AstLiteralExpr& node);
    virtual void visit(AstBinaryExpr& node);
    virtual void visit(AstFctCall& node);
};

} // namespace jex
