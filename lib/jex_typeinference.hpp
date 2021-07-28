#pragma once

#include <jex_basicastvisitor.hpp>

namespace jex {
class CompileEnv;

class TypeInference : public BasicAstVisitor {
    CompileEnv& d_env;
public:
    TypeInference(CompileEnv& env)
    : d_env(env) {
    }

    void visit(AstLiteralExpr& node) override;
    void visit(AstFctCall& node) override;
    void visit(AstIdentifier& node) override;
};

} // namespace jex
