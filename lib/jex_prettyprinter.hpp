#pragma once

#include <jex_astvisitor.hpp>

#include <iosfwd>

namespace jex {

class PrettyPrinter : public IAstVisitor {
    std::ostream& d_str;
public:
    PrettyPrinter(std::ostream& str)
    : d_str(str) {
    }

    void visit(AstLiteralExpr& node) override;
    void visit(AstBinaryExpr& node) override;
    void visit(AstFctCall& node) override;
};

} // namespace jex
