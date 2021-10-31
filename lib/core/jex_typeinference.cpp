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
        case OpType::EQ:
            return "operator_eq";
        case OpType::NE:
            return "operator_ne";
        case OpType::LT:
            return "operator_lt";
        case OpType::GT:
            return "operator_gt";
        case OpType::LE:
            return "operator_le";
        case OpType::GE:
            return "operator_ge";
        case OpType::UMinus:
            return "operator_uminus";
        case OpType::BitAnd:
            return "operator_bitand";
        case OpType::BitOr:
            return "operator_bitor";
        case OpType::BitXor:
            return "operator_bitxor";
        case OpType::Shl:
            return "operator_shl";
        case OpType::Shrs:
            return "operator_shrs";
        case OpType::Shrz:
            return "operator_shrz";
    }
    throw InternalError("Unsupported operator in TypeInference::opTypeToString"); // LCOV_EXCL_LINE
}

void TypeInference::run() {
    d_env.getRoot()->accept(*this);
    if (d_env.hasErrors()) {
        // Throw first error in list.
        throw CompileError::create(*d_env.messages().begin());
    }
}

void TypeInference::visit(AstLiteralExpr& node) {
    BasicAstVisitor::visit(node);
    // Literals are already resolved by the Parser.
    assert(d_env.typeSystem().isResolved(node.d_resultType));
}

bool TypeInference::resolveArguments(const AstFctCall& call, std::vector<TypeInfoId>& argTypes) {
    bool hasUnresolved = false;
    argTypes.reserve(call.d_args->d_args.size());
    for (const IAstExpression* expr : call.d_args->d_args) {
        if (!d_env.typeSystem().isResolved(expr->d_resultType)) {
            // There is already a type inference error, don't report errors resulting from that.
            assert(d_env.hasErrors());
            hasUnresolved = true;
        }
        argTypes.push_back(expr->d_resultType);
    }
    return !hasUnresolved;
}

void TypeInference::visit(AstFctCall& node) {
    BasicAstVisitor::visit(node); // resolve arguments
    std::vector<TypeInfoId> argTypes;
    if (resolveArguments(node, argTypes)) {
        node.d_fctInfo = resolveFct(node, node.d_fct->d_name, argTypes);
    }
}

void TypeInference::visit(AstIf& node) {
    BasicAstVisitor::visit(node); // resolve arguments
    std::vector<TypeInfoId> argTypes;
    if (!resolveArguments(node, argTypes)) {
        return;
    }
    if (argTypes.size() != 3) {
        std::stringstream errMsg;
        errMsg << "'if' function requires exactly 3 arguments, " << argTypes.size() << " given";
        d_env.createError(node.d_loc, errMsg.str());
        return;
    }
    if (argTypes[0] != d_env.typeSystem().getType("Bool")) {
        d_env.createError(node.d_loc,
            "'if' function requires first argument to be of type 'Bool', '"
                + argTypes[0]->name() + "' given");
    }
    if (argTypes[1] != argTypes[2]) {
        d_env.createError(node.d_loc,
            "'if' function requires second and third argument to have the same type, '"
                + argTypes[1]->name() + "' and '" + argTypes[2]->name() + "' given");
    }
    // Set result type to option type.
    // Differently to regular functions, all types are supported.
    // (There isn't any overload resolution for if.)
    node.d_resultType = argTypes[1];
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

void TypeInference::visit(AstUnaryExpr& node) {
    BasicAstVisitor::visit(node); // resolve arguments
    TypeInfoId innerType = node.d_expr->d_resultType;
    if (!d_env.typeSystem().isResolved(innerType)) {
        // There is already a type inference error, don't report errors resulting from that.
        assert(d_env.hasErrors());
        return;
    }
    const char* fctName = opTypeToString(node.d_op);
    node.d_fctInfo = resolveFct(node, fctName, {innerType});
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
        d_env.createError(node.d_loc, errMsg.str());
    }
}

} // namespace jex
