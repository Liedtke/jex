#pragma once

#include <jex_base.hpp>
#include <jex_basicastvisitor.hpp>

#include <string_view>
#include <vector>

namespace jex {
class CompileEnv;
class FctInfo;
class IAstExpression;
class TypeInfoId;

class TypeInference : public BasicAstVisitor, NoCopy {
    CompileEnv& d_env;
public:
    TypeInference(CompileEnv& env)
    : d_env(env) {
    }

    void visit(AstLiteralExpr& node) override;
    void visit(AstFctCall& node) override;
    void visit(AstIf& node) override;
    void visit(AstBinaryExpr& node) override;
    void visit(AstUnaryExpr& node) override;
    void visit(AstVariableDef& node) override;

    void run();

private:
    const FctInfo* resolveFct(IAstExpression& node, std::string_view name, const std::vector<TypeInfoId>& paramTypes);

    /**
     * Resolves arguments and returns true if successful. Returns false if any of the arguments is unresolved.
     */
    bool resolveArguments(const AstFctCall& call, std::vector<TypeInfoId>& argTypes);
};

} // namespace jex
