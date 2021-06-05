#pragma once

namespace jex {

class AstBinaryExpr;
class AstLiteralExpr;
class AstFctCall;

class IAstVisitor {
public:
    virtual ~IAstVisitor() {}

    virtual void visit(AstLiteralExpr& node) = 0;
    virtual void visit(AstBinaryExpr& node) = 0;
    virtual void visit(AstFctCall& node) = 0;
};

} // namespace jex
