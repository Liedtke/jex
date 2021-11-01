#pragma once

#include <jex_basicastvisitor.hpp>
#include <jex_base.hpp>

#include <iosfwd>

namespace jex {

class PrettyPrinter : public BasicAstVisitor, NoCopy {
    std::ostream& d_str;
public:
    PrettyPrinter(std::ostream& str)
    : d_str(str) {
    }

    void visit(AstLiteralExpr& node) override;
    void visit(AstBinaryExpr& node) override;
    void visit(AstUnaryExpr& node) override;
    void visit(AstFctCall& node) override;
    void visit(AstIf& node) override;
    void visit(AstIdentifier& node) override;
    void visit(AstArgList& node) override;
    void visit(AstVariableDef& node) override;
    void visit(AstConstantExpr& node) override;
    void visit(AstVarArg& node) override;
    void visit(AstRoot& node) override;
};

} // namespace jex
