#include <jex_typeinference.hpp>

#include <jex_ast.hpp>
#include <jex_compileenv.hpp>
#include <jex_errorhandling.hpp>
#include <jex_fctinfo.hpp>
#include <jex_fctlibrary.hpp>
#include <jex_symboltable.hpp>
#include <jex_typesystem.hpp>

#include <cassert>

namespace jex {

void TypeInference::visit(AstLiteralExpr& node) {
    BasicAstVisitor::visit(node);
    // Literals are already resolved by the Parser.
    assert(d_env.typeSystem().isResolved(node.d_resultType));
}

void TypeInference::visit(AstFctCall& node) {
    BasicAstVisitor::visit(node); // resolve arguments
    std::vector<TypeInfoId> argTypes;
    argTypes.reserve(node.d_args->d_args.size());
    for (const IAstExpression* expr : node.d_args->d_args) {
        if (!d_env.typeSystem().isResolved(expr->d_resultType)) {
            // There is already a type inference error, don't report errors resulting from that.
            assert(!d_env.messages().empty());
            return;
        }
        argTypes.push_back(expr->d_resultType);
    }
    node.d_fctInfo = resolveFct(node, node.d_fct->d_name, argTypes);
}

void TypeInference::visit(AstBinaryExpr& node) {

}

const FctInfo* TypeInference::resolveFct(IAstExpression& node, std::string_view name, const std::vector<TypeInfoId>& paramTypes) {
    try {
        const FctInfo& fctInfo = d_env.fctLibrary().getFct(std::string(name), paramTypes);
        node.d_resultType = fctInfo.d_retType;
        return &fctInfo;
    } catch (InternalError& err) {
        // Convert exception to non-critical error and add location information.
        d_env.createError(node.d_loc, err.what());
        return nullptr;
    }
}

} // namespace jex
