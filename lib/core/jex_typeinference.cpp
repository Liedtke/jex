#include <jex_typeinference.hpp>

#include <jex_ast.hpp>
#include <jex_compileenv.hpp>
#include <jex_errorhandling.hpp>
#include <jex_fctinfo.hpp>
#include <jex_fctlibrary.hpp>
#include <jex_symboltable.hpp>
#include <jex_typesystem.hpp>

#include <cassert>
#include <sstream>

namespace jex {

static const char* opTypeToString(OpType op) {
    switch (op) {
        case OpType::Add:
            return "operator_add";
        case OpType::Sub:
            return "operator_sub";
        case OpType::Mul:
            return "operator_mul";
        case OpType::Div:
            return "operator_div";
        case OpType::Mod:
            return "operator_mod";
    }
    throw InternalError("Unsupported operator in TypeInference::opTypeToString"); // LCOV_EXCL_LINE
}

void TypeInference::run() {
    d_env.getRoot()->accept(*this);
}

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
            assert(d_env.hasErrors());
            return;
        }
        argTypes.push_back(expr->d_resultType);
    }
    node.d_fctInfo = resolveFct(node, node.d_fct->d_name, argTypes);
}

void TypeInference::visit(AstBinaryExpr& node) {
    BasicAstVisitor::visit(node); // resolve arguments
    std::vector<TypeInfoId> argTypes = {
        node.d_lhs->d_resultType,
        node.d_rhs->d_resultType
    };
    const TypeSystem& typeSystem = d_env.typeSystem();
    if (!typeSystem.isResolved(argTypes[0]) || !typeSystem.isResolved(argTypes[1])) {
        // There is already a type inference error, don't report errors resulting from that.
        assert(d_env.hasErrors());
        return;
    }
    const char* fctName = opTypeToString(node.d_op);
    node.d_fctInfo = resolveFct(node, fctName, argTypes);
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

void TypeInference::visit(AstVariableDef& node) {
    BasicAstVisitor::visit(node); // resolve expression
    TypeInfoId exprType = node.d_expr->d_resultType;
    if (d_env.typeSystem().isResolved(exprType) && node.d_resultType != exprType) {
        std::stringstream errMsg;
        errMsg << "Invalid type for variable '" << node.d_name->d_name
               << "': Specified as '" << node.d_resultType->name()
               << "' but expression returns '" << exprType->name() << "'";
    }
}

} // namespace jex
