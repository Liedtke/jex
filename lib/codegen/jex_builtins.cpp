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

} // anonymous namespace

void BuiltInsModule::registerTypes(Registry& registry) const {
    registry.registerType<ArgBool>(
        [](llvm::LLVMContext& ctx) { return llvm::Type::getInt1Ty(ctx); });
    registry.registerType<ArgInteger>(
        [](llvm::LLVMContext& ctx) { return llvm::Type::getInt64Ty(ctx); });
    registry.registerType<ArgFloat>(
        [](llvm::LLVMContext& ctx) { return llvm::Type::getDoubleTy(ctx); });
}

void BuiltInsModule::registerFcts(Registry& registry) const {
    // === Bool ===
    // Comparisons
    using BoolCmp = FctDesc<ArgBool, ArgBool, ArgBool>;
    using IntegerCmpIntr = CmpIntrinsics<llvm::ICmpInst>;
    registry.registerFct(BoolCmp("operator_eq", cmp<std::equal_to<>>, IntegerCmpIntr::generate<llvm::CmpInst::Predicate::ICMP_EQ>));
    registry.registerFct(BoolCmp("operator_ne", cmp<std::not_equal_to<>>, IntegerCmpIntr::generate<llvm::CmpInst::Predicate::ICMP_NE>));

    // === Integer ===
    // Arithmetics
    using IntegerArithm = FctDesc<ArgInteger, ArgInteger, ArgInteger>;
    registry.registerFct(IntegerArithm("operator_add", op<std::plus<>>, generateOp<llvm::BinaryOperator::Add>));
    registry.registerFct(IntegerArithm("operator_sub", op<std::minus<>>, generateOp<llvm::BinaryOperator::Sub>));
    registry.registerFct(IntegerArithm("operator_mul", op<std::multiplies<>>, generateOp<llvm::BinaryOperator::Mul>));
    registry.registerFct(IntegerArithm("operator_div", op<std::divides<>>, generateOp<llvm::BinaryOperator::SDiv>));
    registry.registerFct(IntegerArithm("operator_mod", op<std::modulus<>>, generateOp<llvm::BinaryOperator::SRem>));
    // Comparisons
    using IntegerCmp = FctDesc<ArgBool, ArgInteger, ArgInteger>;
    registry.registerFct(IntegerCmp("operator_eq", cmp<std::equal_to<>>, IntegerCmpIntr::generate<llvm::CmpInst::Predicate::ICMP_EQ>));
    registry.registerFct(IntegerCmp("operator_ne", cmp<std::not_equal_to<>>, IntegerCmpIntr::generate<llvm::CmpInst::Predicate::ICMP_NE>));
    registry.registerFct(IntegerCmp("operator_lt", cmp<std::less<>>, IntegerCmpIntr::generate<llvm::CmpInst::Predicate::ICMP_SLT>));
    registry.registerFct(IntegerCmp("operator_gt", cmp<std::greater<>>, IntegerCmpIntr::generate<llvm::CmpInst::Predicate::ICMP_SGT>));
    registry.registerFct(IntegerCmp("operator_le", cmp<std::less_equal<>>, IntegerCmpIntr::generate<llvm::CmpInst::Predicate::ICMP_SLE>));
    registry.registerFct(IntegerCmp("operator_ge", cmp<std::greater_equal<>>, IntegerCmpIntr::generate<llvm::CmpInst::Predicate::ICMP_SGE>));

    // === Float ===
    // Arithmetics
    using FloatArithm = FctDesc<ArgFloat, ArgFloat, ArgFloat>;
    registry.registerFct(FloatArithm("operator_add", op<std::plus<>>, generateOp<llvm::BinaryOperator::FAdd>));
    registry.registerFct(FloatArithm("operator_sub", op<std::minus<>>, generateOp<llvm::BinaryOperator::FSub>));
    registry.registerFct(FloatArithm("operator_mul", op<std::multiplies<>>, generateOp<llvm::BinaryOperator::FMul>));
    registry.registerFct(FloatArithm("operator_div", op<std::divides<>>, generateOp<llvm::BinaryOperator::FDiv>));
    // Comparisons
    using FloatCmp = FctDesc<ArgBool, ArgFloat, ArgFloat>;
    using FloatCmpIntr = CmpIntrinsics<llvm::FCmpInst>;
    registry.registerFct(FloatCmp("operator_eq", cmp<std::equal_to<>>, FloatCmpIntr::generate<llvm::CmpInst::Predicate::FCMP_OEQ>));
    registry.registerFct(FloatCmp("operator_ne", cmp<std::not_equal_to<>>, FloatCmpIntr::generate<llvm::CmpInst::Predicate::FCMP_ONE>));
    registry.registerFct(FloatCmp("operator_lt", cmp<std::less<>>, FloatCmpIntr::generate<llvm::CmpInst::Predicate::FCMP_OLT>));
    registry.registerFct(FloatCmp("operator_gt", cmp<std::greater<>>, FloatCmpIntr::generate<llvm::CmpInst::Predicate::FCMP_OGT>));
    registry.registerFct(FloatCmp("operator_le", cmp<std::less_equal<>>, FloatCmpIntr::generate<llvm::CmpInst::Predicate::FCMP_OLE>));
    registry.registerFct(FloatCmp("operator_ge", cmp<std::greater_equal<>>, FloatCmpIntr::generate<llvm::CmpInst::Predicate::FCMP_OGE>));
}

} // namespace jex