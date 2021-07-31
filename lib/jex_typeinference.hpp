#pragma once

#include <jex_basicastvisitor.hpp>

#include <string_view>
#include <vector>

namespace jex {
class CompileEnv;
class FctInfo;
class IAstExpression;
class TypeInfoId;

class TypeInference : public BasicAstVisitor {
    CompileEnv& d_env;
public:
    TypeInference(CompileEnv& env)
    : d_env(env) {
    }

    void visit(AstLiteralExpr& node) override;
    void visit(AstFctCall& node) override;
    void visit(AstBinaryExpr& node) override;

private:
    const FctInfo* resolveFct(IAstExpression& node, std::string_view name, const std::vector<TypeInfoId>& paramTypes);
};

} // namespace jex
