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
void cmp(bool* res, T a, T b) {
    assert(res != nullptr);
    *res = Op()(a, b);
}

template <typename Op, typename T>
void unaryOp(T* res, T arg) {
    assert(res != nullptr);
    *res = Op()(arg);
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
    registry.registerFct(FctDesc<ArgInteger, ArgInteger>("operator_uminus", unaryOp<std::negate<>>, generateUnaryNeg, FctFlags::Pure));
    // Comparisons
    using IntegerCmp = FctDesc<ArgBool, ArgInteger, ArgInteger>;
    registry.registerFct(IntegerCmp("operator_eq", cmp<std::equal_to<>>, IntegerCmpIntr::generate<llvm::CmpInst::Predicate::ICMP_EQ>, FctFlags::Pure));
    registry.registerFct(IntegerCmp("operator_ne", cmp<std::not_equal_to<>>, IntegerCmpIntr::generate<llvm::CmpInst::Predicate::ICMP_NE>, FctFlags::Pure));
    registry.registerFct(IntegerCmp("operator_lt", cmp<std::less<>>, IntegerCmpIntr::generate<llvm::CmpInst::Predicate::ICMP_SLT>, FctFlags::Pure));
    registry.registerFct(IntegerCmp("operator_gt", cmp<std::greater<>>, IntegerCmpIntr::generate<llvm::CmpInst::Predicate::ICMP_SGT>, FctFlags::Pure));
    registry.registerFct(IntegerCmp("operator_le", cmp<std::less_equal<>>, IntegerCmpIntr::generate<llvm::CmpInst::Predicate::ICMP_SLE>, FctFlags::Pure));
    registry.registerFct(IntegerCmp("operator_ge", cmp<std::greater_equal<>>, IntegerCmpIntr::generate<llvm::CmpInst::Predicate::ICMP_SGE>, FctFlags::Pure));

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

    // === String ===
    registry.registerFct(FctDesc<ArgString, ArgString, ArgInteger, ArgInteger>("substr", substr, NO_INTRINSIC, FctFlags::Pure));
}

} // namespace jex
