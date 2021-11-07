#include <jex_builtins.hpp>
#include <jex_intrinsicgen.hpp>

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"

#include <cassert>
#include <functional>

namespace jex {

namespace {

// T* return type could also be replaced with decltype(Op()(T(), T())) if desired.
// However, T* ensures consistency between return type and type of inputs.
template <typename Op, typename T>
void op(T* res, T a, T b) {
    assert(res != nullptr);
    *res = Op()(a, b);
}

template <typename Op, typename T>
void cmpPtr(bool* res, const T* a, const T* b) {
    assert(res != nullptr);
    *res = Op()(*a, *b);
}

template <typename Op, typename T>
void cmp(bool* res, T a, T b) {
    assert(res != nullptr);
    *res = Op()(a, b);
}

template <typename Op, typename T>
void unaryOp(T* res, T arg) {
    assert(res != nullptr);
    *res = Op()(arg);
}

void shiftLeft(int64_t* res, int64_t val, int64_t shiftBy) {
    assert(res != nullptr);
    assert(shiftBy >= 0);
    assert(shiftBy < 64);
    *res = val << shiftBy;
}

void shiftRightSigned(int64_t* res, int64_t val, int64_t shiftBy) {
    assert(res != nullptr);
    assert(shiftBy >= 0);
    assert(shiftBy < 64);
    auto uval = static_cast<uint64_t>(val);
    if (val < 0) {
        *res = static_cast<int64_t>(uval >> shiftBy | ~(~0ULL >> shiftBy));
    } else {
        *res = static_cast<int64_t>(uval >> shiftBy);
    }
}

void shiftRightZero(int64_t* res, int64_t val, int64_t shiftBy) {
    assert(res != nullptr);
    assert(shiftBy >= 0);
    assert(shiftBy < 64);
    *res = static_cast<int64_t>(static_cast<uint64_t>(val) >> shiftBy);
}

template <typename T>
void max(T* res, const VarArg<T>* args) {
    assert(args->size() != 0);
    T maxVal = *args->begin();
    for (T val : *args) {
        if (!(val < maxVal)) {
            maxVal = val;
        }
    }
    *res = maxVal;
}

void join(std::string* res, const std::string* separator, const VarArg<const std::string*>* args) {
    size_t cap = (args->size() - 1) * separator->size() + 1; // +1 for null-terminator
    for (const std::string* str : *args) {
        cap += str->size();
    }
    new(res) std::string();
    res->reserve(cap);
    bool first = true;
    for (const std::string* str : *args) {
        if (first) {
            first = false;
        } else {
            *res += *separator;
        }
        *res += *str;
    }
}

void generateMax(IntrinsicGen& gen) {
    llvm::Type* elemType = gen.fct().getArg(0)->getType()->getPointerElementType();
    llvm::IRBuilder<>& builder = gen.builder();
    // Extract argc and array pointer from vararg struct.
    llvm::Value* varArg = gen.fct().getArg(1);
    llvm::Value* idx0 = llvm::ConstantInt::get(gen.llvmContext(), llvm::APInt(32, 0));
    llvm::Value* idx1 = llvm::ConstantInt::get(gen.llvmContext(), llvm::APInt(32, 1));
    llvm::Value* argcPtr = builder.CreateInBoundsGEP(varArg, {idx0, idx1}, "argcPtr");
    llvm::Value* argc = builder.CreateLoad(argcPtr, "argc");
    llvm::Value* arrayBegin = builder.CreateLoad(builder.CreateGEP(varArg, {idx0, idx0}), "arrayBegin");
    llvm::Value* arrayEnd = builder.CreateGEP(arrayBegin, argc, "arrayEnd");
    // Set max to first element.
    llvm::Value* firstVal = builder.CreateLoad(arrayBegin, "firstVal");
    llvm::Value* secondElemPtr = builder.CreateGEP(arrayBegin, idx1, "secondElemPtr");
    // Check if there is more than one element.
    llvm::Value* hasMultiElems = builder.CreateICmpNE(argc, llvm::ConstantInt::get(gen.llvmContext(), llvm::APInt(64, 1)));
    auto* blockLoopEntry = llvm::BasicBlock::Create(gen.llvmContext(), "loopEntry", &gen.fct());
    auto* blockExit = llvm::BasicBlock::Create(gen.llvmContext(), "loopExit", &gen.fct());
    llvm::BasicBlock* initBlock = builder.GetInsertBlock();
    builder.CreateCondBr(hasMultiElems, blockLoopEntry, blockExit);
    // Generate loop.
    builder.SetInsertPoint(blockLoopEntry);
    llvm::PHINode* elemPtrPhi = builder.CreatePHI(arrayBegin->getType(), 2, "elemPtr");
    elemPtrPhi->addIncoming(secondElemPtr, initBlock);
    llvm::PHINode* maxValPhi = builder.CreatePHI(elemType, 2, "maxVal");
    maxValPhi->addIncoming(firstVal, initBlock);
    llvm::Value* val = builder.CreateLoad(elemPtrPhi, "val");
    llvm::Value* isGreater = builder.CreateICmpSGT(val, maxValPhi, "valIsGreater");
    llvm::Value* newMax = builder.CreateSelect(isGreater, val, maxValPhi, "newMax");
    maxValPhi->addIncoming(newMax, blockLoopEntry);
    llvm::Value* nextElemPtr = builder.CreateGEP(elemPtrPhi, idx1, "nextElemPtr");
    elemPtrPhi->addIncoming(nextElemPtr, blockLoopEntry);
    llvm::Value* hasMore = builder.CreateICmpNE(nextElemPtr, arrayEnd, "hasMore");
    builder.CreateCondBr(hasMore, blockLoopEntry, blockExit);
    // End of loop: Store maxVal in result.
    builder.SetInsertPoint(blockExit);
    llvm::PHINode* maxVal = builder.CreatePHI(elemType, 2, "finalMax");
    maxVal->addIncoming(firstVal, initBlock);
    maxVal->addIncoming(newMax, blockLoopEntry);
    builder.CreateStore(maxVal, gen.fct().getArg(0));
}

template <llvm::Instruction::BinaryOps op>
void generateOp(IntrinsicGen& gen) {
    llvm::IRBuilder<>& builder = gen.builder();
    llvm::Value* result = llvm::BinaryOperator::Create(
        op, gen.fct().getArg(1), gen.fct().getArg(2), "result", builder.GetInsertBlock());
    builder.CreateStore(result, gen.fct().getArg(0));
}

template <typename CmpInst>
struct CmpIntrinsics {
    template <llvm::CmpInst::Predicate predicate>
    static void generate(IntrinsicGen& gen) {
        llvm::IRBuilder<>& builder = gen.builder();
        llvm::Value* result = new CmpInst(
            *builder.GetInsertBlock(), predicate, gen.fct().getArg(1), gen.fct().getArg(2), "result");
        builder.CreateStore(result, gen.fct().getArg(0));
    }
};

void generateUnaryNeg(IntrinsicGen& gen) {
    llvm::IRBuilder<>& builder = gen.builder();
    llvm::Value* result = builder.CreateNeg(gen.fct().getArg(1), "result");
    builder.CreateStore(result, gen.fct().getArg(0));
}

void generateUnaryFNeg(IntrinsicGen& gen) {
    llvm::IRBuilder<>& builder = gen.builder();
    llvm::Value* result = builder.CreateFNeg(gen.fct().getArg(1), "result");
    builder.CreateStore(result, gen.fct().getArg(0));
}

void generateUnaryNot(IntrinsicGen& gen) {
    llvm::IRBuilder<>& builder = gen.builder();
    llvm::Value* result = builder.CreateNot(gen.fct().getArg(1), "result");
    builder.CreateStore(result, gen.fct().getArg(0));
}

void substr(std::string* res, const std::string* in, int64_t pos, int64_t count) {
    assert(res != nullptr);
    assert(pos >= 0); // TODO: Figure out how to support error handling in expressions.
    assert(count >= 0);
    new (res) std::string(in->substr(pos, count));
}

} // anonymous namespace

void BuiltInsModule::registerTypes(Registry& registry) const {
    bool zeroInitialized = true;
    registry.registerType<ArgBool>(
        [](llvm::LLVMContext& ctx) { return llvm::Type::getInt1Ty(ctx); }, zeroInitialized);
    registry.registerType<ArgInteger>(
        [](llvm::LLVMContext& ctx) { return llvm::Type::getInt64Ty(ctx); }, zeroInitialized);
    registry.registerType<ArgFloat>(
        [](llvm::LLVMContext& ctx) { return llvm::Type::getDoubleTy(ctx); }, zeroInitialized);
    registry.registerType<ArgString>();
}

void BuiltInsModule::registerFcts(Registry& registry) const {
    // === Bool ===
    // Comparisons
    using BoolCmp = FctDesc<ArgBool, ArgBool, ArgBool>;
    using IntegerCmpIntr = CmpIntrinsics<llvm::ICmpInst>;
    registry.registerFct(BoolCmp("operator_eq", cmp<std::equal_to<>>, IntegerCmpIntr::generate<llvm::CmpInst::Predicate::ICMP_EQ>, FctFlags::Pure));
    registry.registerFct(BoolCmp("operator_ne", cmp<std::not_equal_to<>>, IntegerCmpIntr::generate<llvm::CmpInst::Predicate::ICMP_NE>, FctFlags::Pure));
    registry.registerFct(BoolCmp("operator_bitand", op<std::bit_and<>>, generateOp<llvm::BinaryOperator::And>, FctFlags::Pure));
    registry.registerFct(BoolCmp("operator_bitor", op<std::bit_or<>>, generateOp<llvm::BinaryOperator::Or>, FctFlags::Pure));
    registry.registerFct(BoolCmp("operator_bitxor", op<std::bit_xor<>>, generateOp<llvm::BinaryOperator::Xor>, FctFlags::Pure));
    registry.registerFct(FctDesc<ArgBool, ArgBool>("operator_not", unaryOp<std::logical_not<>>, generateUnaryNot, FctFlags::Pure));

    // === Integer ===
    // Arithmetics
    using IntegerArithm = FctDesc<ArgInteger, ArgInteger, ArgInteger>;
    registry.registerFct(IntegerArithm("operator_add", op<std::plus<>>, generateOp<llvm::BinaryOperator::Add>, FctFlags::Pure));
    registry.registerFct(IntegerArithm("operator_sub", op<std::minus<>>, generateOp<llvm::BinaryOperator::Sub>, FctFlags::Pure));
    registry.registerFct(IntegerArithm("operator_mul", op<std::multiplies<>>, generateOp<llvm::BinaryOperator::Mul>, FctFlags::Pure));
    registry.registerFct(IntegerArithm("operator_div", op<std::divides<>>, generateOp<llvm::BinaryOperator::SDiv>, FctFlags::Pure));
    registry.registerFct(IntegerArithm("operator_mod", op<std::modulus<>>, generateOp<llvm::BinaryOperator::SRem>, FctFlags::Pure));
    registry.registerFct(IntegerArithm("operator_bitand", op<std::bit_and<>>, generateOp<llvm::BinaryOperator::And>, FctFlags::Pure));
    registry.registerFct(IntegerArithm("operator_bitor", op<std::bit_or<>>, generateOp<llvm::BinaryOperator::Or>, FctFlags::Pure));
    registry.registerFct(IntegerArithm("operator_bitxor", op<std::bit_xor<>>, generateOp<llvm::BinaryOperator::Xor>, FctFlags::Pure));
    registry.registerFct(IntegerArithm("operator_shl", shiftLeft, generateOp<llvm::BinaryOperator::Shl>, FctFlags::Pure));
    registry.registerFct(IntegerArithm("operator_shrs", shiftRightSigned, generateOp<llvm::BinaryOperator::AShr>, FctFlags::Pure));
    registry.registerFct(IntegerArithm("operator_shrz", shiftRightZero, generateOp<llvm::BinaryOperator::LShr>, FctFlags::Pure));
    registry.registerFct(FctDesc<ArgInteger, ArgInteger>("operator_uminus", unaryOp<std::negate<>>, generateUnaryNeg, FctFlags::Pure));
    // Comparisons
    using IntegerCmp = FctDesc<ArgBool, ArgInteger, ArgInteger>;
    registry.registerFct(IntegerCmp("operator_eq", cmp<std::equal_to<>>, IntegerCmpIntr::generate<llvm::CmpInst::Predicate::ICMP_EQ>, FctFlags::Pure));
    registry.registerFct(IntegerCmp("operator_ne", cmp<std::not_equal_to<>>, IntegerCmpIntr::generate<llvm::CmpInst::Predicate::ICMP_NE>, FctFlags::Pure));
    registry.registerFct(IntegerCmp("operator_lt", cmp<std::less<>>, IntegerCmpIntr::generate<llvm::CmpInst::Predicate::ICMP_SLT>, FctFlags::Pure));
    registry.registerFct(IntegerCmp("operator_gt", cmp<std::greater<>>, IntegerCmpIntr::generate<llvm::CmpInst::Predicate::ICMP_SGT>, FctFlags::Pure));
    registry.registerFct(IntegerCmp("operator_le", cmp<std::less_equal<>>, IntegerCmpIntr::generate<llvm::CmpInst::Predicate::ICMP_SLE>, FctFlags::Pure));
    registry.registerFct(IntegerCmp("operator_ge", cmp<std::greater_equal<>>, IntegerCmpIntr::generate<llvm::CmpInst::Predicate::ICMP_SGE>, FctFlags::Pure));

    registry.registerFct(FctDesc<ArgInteger, ArgVarArg<ArgInteger>>("max", max, generateMax, FctFlags::Pure));

    // === Float ===
    // Arithmetics
    using FloatArithm = FctDesc<ArgFloat, ArgFloat, ArgFloat>;
    registry.registerFct(FloatArithm("operator_add", op<std::plus<>>, generateOp<llvm::BinaryOperator::FAdd>, FctFlags::Pure));
    registry.registerFct(FloatArithm("operator_sub", op<std::minus<>>, generateOp<llvm::BinaryOperator::FSub>, FctFlags::Pure));
    registry.registerFct(FloatArithm("operator_mul", op<std::multiplies<>>, generateOp<llvm::BinaryOperator::FMul>, FctFlags::Pure));
    registry.registerFct(FloatArithm("operator_div", op<std::divides<>>, generateOp<llvm::BinaryOperator::FDiv>, FctFlags::Pure));
    registry.registerFct(FctDesc<ArgFloat, ArgFloat>("operator_uminus", unaryOp<std::negate<>>, generateUnaryFNeg, FctFlags::Pure));
    // Comparisons
    using FloatCmp = FctDesc<ArgBool, ArgFloat, ArgFloat>;
    using FloatCmpIntr = CmpIntrinsics<llvm::FCmpInst>;
    registry.registerFct(FloatCmp("operator_eq", cmp<std::equal_to<>>, FloatCmpIntr::generate<llvm::CmpInst::Predicate::FCMP_OEQ>, FctFlags::Pure));
    registry.registerFct(FloatCmp("operator_ne", cmp<std::not_equal_to<>>, FloatCmpIntr::generate<llvm::CmpInst::Predicate::FCMP_ONE>, FctFlags::Pure));
    registry.registerFct(FloatCmp("operator_lt", cmp<std::less<>>, FloatCmpIntr::generate<llvm::CmpInst::Predicate::FCMP_OLT>, FctFlags::Pure));
    registry.registerFct(FloatCmp("operator_gt", cmp<std::greater<>>, FloatCmpIntr::generate<llvm::CmpInst::Predicate::FCMP_OGT>, FctFlags::Pure));
    registry.registerFct(FloatCmp("operator_le", cmp<std::less_equal<>>, FloatCmpIntr::generate<llvm::CmpInst::Predicate::FCMP_OLE>, FctFlags::Pure));
    registry.registerFct(FloatCmp("operator_ge", cmp<std::greater_equal<>>, FloatCmpIntr::generate<llvm::CmpInst::Predicate::FCMP_OGE>, FctFlags::Pure));

    registry.registerFct(FctDesc<ArgFloat, ArgVarArg<ArgFloat>>("max", max, NO_INTRINSIC, FctFlags::Pure));

    // === String ===
    registry.registerFct(FctDesc<ArgString, ArgString, ArgInteger, ArgInteger>("substr", substr, NO_INTRINSIC, FctFlags::Pure));
    registry.registerFct(FctDesc<ArgString, ArgString, ArgVarArg<ArgString>>("join", join, NO_INTRINSIC, FctFlags::Pure));
    registry.registerFct(FctDesc<ArgBool, ArgString, ArgString>("operator_eq", cmpPtr<std::equal_to<>>, NO_INTRINSIC, FctFlags::Pure));
    registry.registerFct(FctDesc<ArgBool, ArgString, ArgString>("operator_ne", cmpPtr<std::not_equal_to<>>, NO_INTRINSIC, FctFlags::Pure));
    registry.registerFct(FctDesc<ArgBool, ArgString, ArgString>("operator_lt", cmpPtr<std::less<>>, NO_INTRINSIC, FctFlags::Pure));
    registry.registerFct(FctDesc<ArgBool, ArgString, ArgString>("operator_gt", cmpPtr<std::greater<>>, NO_INTRINSIC, FctFlags::Pure));
    registry.registerFct(FctDesc<ArgBool, ArgString, ArgString>("operator_le", cmpPtr<std::less_equal<>>, NO_INTRINSIC, FctFlags::Pure));
    registry.registerFct(FctDesc<ArgBool, ArgString, ArgString>("operator_ge", cmpPtr<std::greater_equal<>>, NO_INTRINSIC, FctFlags::Pure));
}

} // namespace jex
