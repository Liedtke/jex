#include <jex_constantfolding.hpp>

#include <jex_ast.hpp>
#include <jex_compileenv.hpp>
#include <jex_errorhandling.hpp>
#include <jex_fctinfo.hpp>
#include <jex_fctlibrary.hpp>

#include <cstring>

namespace jex {

void* ConstantOrLiteral::getPtr() {
    if (d_constant.getPtr() == nullptr) {
        assert(isLiteral() && "object must contain either a literal or a constant");
        // Create Constant for storing the actual object. (AstLiteralExpr doesn't contain it.)
        return std::visit(overloaded {
            [&](std::string_view val) {
                using String = std::string;
                auto dtor = [](void* str) { reinterpret_cast<std::string*>(str)->~String(); };
                d_constant = Constant::create(std::string(val), dtor);
                return d_constant.getPtr();
            },
            [&](auto& val) -> void* { return &val; },
        }, d_literal->d_value);
    }
    return d_constant.getPtr();
}

Constant ConstantOrLiteral::release() {
    assert(!isLiteral());
    return std::move(d_constant);
}

Constant& ConstantOrLiteral::getConstant() {
    assert(!isLiteral());
    return d_constant;
}

void ConstantFolding::run() {
    d_env.getRoot()->accept(*this);
    if (d_env.hasErrors()) {
        // Throw first error in list.
        throw CompileError::create(*d_env.messages().begin());
    }
}

bool ConstantFolding::tryFold(IAstExpression*& expr) {
    assert(d_foldedExpr == nullptr);
    expr->accept(*this);
    if (d_foldedExpr != nullptr) {
        // Replace node in AST with constant expression.
        expr = std::exchange(d_foldedExpr, nullptr);
    }
    return expr->isConstant();
}

void* ConstantFolding::getPtrFor(IAstExpression* expr) {
    auto iter = d_constants.find(expr);
    assert(iter != d_constants.end() && "trying to get constant for not folded expression");
    return iter->second.getPtr();
}

void ConstantFolding::storeIfConstant(IAstExpression* expr) {
    auto iter = d_constants.find(expr);
    if (iter != d_constants.end() && !iter->second.isLiteral()) {
        auto asConstExpr = cast_ensured<AstConstantExpr*>(expr);
        d_env.constants().insert(asConstExpr->d_constantName, iter->second.release());
    }
}

bool ConstantFolding::tryFoldAndStore(IAstExpression*& expr) {
    if (tryFold(expr)) {
        storeIfConstant(expr);
        return true;
    }
    return false;
}

void ConstantFolding::foldFunctionCall(IAstExpression& callExpr, const FctInfo& fctInfo,
                                       const std::vector<IAstExpression*>& args) {
    AstConstantExpr* constNode = d_env.createNode<AstConstantExpr>(callExpr);
        [[maybe_unused]] auto[iterator, inserted] =
            d_constants.emplace(constNode, Constant::allocate(callExpr.d_resultType->size()));
        assert(inserted);
        // Evaluate function.
        std::vector<void*> argPtrs;
        argPtrs.reserve(1 + args.size());
        argPtrs.push_back(iterator->second.getPtr());
        for (IAstExpression* expr : args) {
            argPtrs.push_back(getPtrFor(expr));
        }
        fctInfo.call(argPtrs.data());
        // Set destructor for memory management.
        if (callExpr.d_resultType->kind() == TypeKind::Complex) {
            void* fctPtr = d_env.fctLibrary().getDestructor(callExpr.d_resultType).d_fctPtr;
            iterator->second.getConstant().dtor = reinterpret_cast<Constant::Dtor>(fctPtr);
        }
        d_foldedExpr = constNode;
}

void ConstantFolding::visit(AstBinaryExpr& node) {
    bool isConst = tryFold(node.d_lhs) & tryFold(node.d_rhs);
    if (isConst && node.d_fctInfo->isPure()) {
        foldFunctionCall(node, *node.d_fctInfo, {node.d_lhs, node.d_rhs});
    } else {
        // Move inner constants to permanent constant store if any.
        storeIfConstant(node.d_lhs);
        storeIfConstant(node.d_rhs);
    }
}

void ConstantFolding::visit(AstLogicalBinExpr& node) {
    bool lhsIsConst = tryFold(node.d_lhs);
    if (lhsIsConst) {
        auto iter = d_constants.find(node.d_lhs);
        bool lhsValue = *reinterpret_cast<bool*>(iter->second.getPtr());
        if ((node.d_op == OpType::Or) == lhsValue) {
            // true  || ... --> true
            // false && ... --> false
            d_foldedExpr = node.d_lhs; // node.d_lhs == false!
        } else {
            // true  && ... --> ...
            // false || ... --> ...
            tryFold(node.d_rhs);
            d_foldedExpr = node.d_rhs;
        }
    }
}

void ConstantFolding::visit(AstUnaryExpr& node) {
    bool isConst = tryFold(node.d_expr);
    if (isConst && node.d_fctInfo->isPure()) {
        foldFunctionCall(node, *node.d_fctInfo, {node.d_expr});
    } else {
        // Move inner constants to permanent constant store if any.
        storeIfConstant(node.d_expr);
    }
}

void ConstantFolding::visit(AstFctCall& node) {
    // Fold all arguments.
    bool isConst = true;
    for (IAstExpression*& arg : node.d_args->d_args) {
        isConst &= tryFold(arg);
    }
    // Fold call itself.
    if (isConst && node.d_fctInfo->isPure()) {
        foldFunctionCall(node, *node.d_fctInfo, node.d_args->d_args);
    } else {
        // Move inner constants to permanent constant store if any.
        for (IAstExpression* arg : node.d_args->d_args) {
            storeIfConstant(arg);
        }
    }
}

void ConstantFolding::visit(AstLiteralExpr& node) {
    d_constants.emplace(&node, ConstantOrLiteral(node));
}

void ConstantFolding::visit(AstIf& node) {
    bool isConstCond = tryFold(node.d_args->d_args[0]);
    if (!isConstCond) {
        // Try constant folding on both sides.
        tryFoldAndStore(node.d_args->d_args[1]);
        tryFoldAndStore(node.d_args->d_args[2]);
        return;
    }
    const bool condValue = *reinterpret_cast<bool*>(getPtrFor(node.d_args->d_args[0]));
    IAstExpression*& expr = node.d_args->d_args[condValue ? 1 : 2];
    tryFold(expr);
    // Even if 'expr' isn't const, this is going to replace the AstIf with 'expr'.
    d_foldedExpr = expr;
}

void ConstantFolding::visit(AstIdentifier& node) {
    // Never const currently.
}

void ConstantFolding::visit(AstVariableDef& node) {
    if (d_foldAll || node.d_kind == VariableKind::Const) {
        tryFoldAndStore(node.d_expr);
    }
    if (node.d_kind == VariableKind::Const && !node.d_expr->isConstant()) {
        d_env.createError(node.d_expr, "Right hand side of constant '" + std::string(node.d_name->d_name) + "' is not a constant expression");
    }
}

void ConstantFolding::visit(AstVarArg& node) {
    bool isConst = true;
    for (IAstExpression*& arg : node.d_args) {
        isConst &= tryFold(arg);
    }
    if (isConst) {
        // Allocate Constant holding vararg object and array.
        const size_t varArgStructSize = sizeof(VarArg<void>);
        const TypeInfoId elemType = node.d_args[0]->d_resultType;
        const bool byValue = elemType->callConv() == TypeInfo::CallConv::ByValue;
        const size_t elemSize = byValue ? elemType->size() : sizeof(void*);
        const size_t elemAlign = byValue ? elemType->alignment() : alignof(void*);
        const size_t arraySize = elemSize * node.d_args.size();
        size_t allocSizeForArray = elemAlign + arraySize;
        // This could be optimized to check for the actually needed alignment gap.
        Constant constant = Constant::allocate(varArgStructSize + allocSizeForArray); // NOLINT
        void* ptr = static_cast<char*>(constant.getPtr()) + varArgStructSize;
        void* arrayPtr = std::align(elemAlign, arraySize, ptr, allocSizeForArray);
        // Initialize VarArg object.
        new (constant.getPtr()) VarArg<void>(arrayPtr, node.d_args.size());
        // Copy arguments over.
        auto elemPtr = static_cast<char*>(arrayPtr);
        for (IAstExpression* arg : node.d_args) {
            assert(arg->d_resultType == elemType);
            if (byValue) {
                // Copy value into array.
                std::memcpy(elemPtr, getPtrFor(arg), elemSize);
            } else {
                // Store pointer to value in array.
                *reinterpret_cast<void**>(elemPtr) = getPtrFor(arg);
            }
            elemPtr += elemSize;
        }
        // Create and store Constant ast node replacing the AstVarArg.
        d_foldedExpr = d_env.createNode<AstConstantExpr>(node);
        d_constants.emplace(d_foldedExpr, std::move(constant));
    }
    for (IAstExpression* arg : node.d_args) {
        // As the VarArg doesn't own the memory, we have to store the constants independently
        // of whether the vararg is constant or not.
        // This means that these constants will also be kept alive even if the whole call gets
        // folded. TODO: Could this be optimized easily?
        storeIfConstant(arg);
    }
}

} // namespace jex
