#pragma once

#include <jex_base.hpp>
#include <jex_basicastvisitor.hpp>
#include <jex_constantstore.hpp>

#include <unordered_map>

namespace jex {

class CompileEnv;

/**
 * Holds a constant or literal and only initializes the constant value if requested by getPtr().
 */
class ConstantOrLiteral {
    Constant d_constant;
    AstLiteralExpr* d_literal = nullptr;
public:
    explicit ConstantOrLiteral(Constant&& constant) : d_constant(std::move(constant)) {
    }
    explicit ConstantOrLiteral(AstLiteralExpr& literal) : d_constant{}, d_literal(&literal) {
    }

    void* getPtr();
    bool isLiteral() const {
        return d_literal != nullptr;
    }
    Constant release();
    Constant& getConstant();
};

class ConstantFolding : public BasicAstVisitor, NoCopy {
    CompileEnv& d_env;
    IAstExpression* d_foldedExpr = nullptr;
    std::unordered_map<IAstExpression*, ConstantOrLiteral> d_constants;
    // Flag whether all expressions supporting const-folding shall be folded.
    // If false, only const variables will be folded.
    const bool d_foldAll;
public:
    explicit ConstantFolding(CompileEnv& env, bool foldAll) : d_env(env), d_foldAll(foldAll) {
    }

    void run();

private:
    void visit(AstLiteralExpr& node) override;
    void visit(AstBinaryExpr& node) override;
    void visit(AstFctCall& node) override;
    void visit(AstIf& node) override;
    void visit(AstIdentifier& node) override;
    void visit(AstVariableDef& node) override;
    void visit(AstVarArg& node) override;

    bool tryFold(IAstExpression*& expr);
    bool tryFoldAndStore(IAstExpression*& expr);
    void* getPtrFor(IAstExpression* expr);
    void storeIfConstant(IAstExpression* expr);
    void foldFunctionCall(IAstExpression& callExpr, const FctInfo& fctInfo,
                          const std::vector<IAstExpression*>& args);
};

} // namespace jex
