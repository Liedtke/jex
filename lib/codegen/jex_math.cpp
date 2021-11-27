#include <jex_math.hpp>

#include <jex_builtins.hpp>
#include <jex_intrinsicgen.hpp>
#include <jex_typehelpers.hpp>

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"

namespace jex {

namespace {

void complexCtor(Complex* res, double real, double imaginary) {
    new (res) Complex(real, imaginary);
}

void complexCtorIntrinsic(IntrinsicGen& gen) {
    llvm::Argument* argRes = gen.fct().getArg(0);
    llvm::IRBuilder<>& builder = gen.builder();
    for (size_t i = 0; i < 2; ++i) {
        llvm::Value* elemPtr = gen.getStructElemPtr(argRes, i);
        llvm::Argument* arg = gen.fct().getArg(i+1);
        builder.CreateStore(arg, elemPtr);
    }
}

void complexAddIntrinsic(IntrinsicGen& gen) {
    llvm::Argument* argRes = gen.fct().getArg(0);
    llvm::Argument* argA = gen.fct().getArg(1);
    llvm::Argument* argB = gen.fct().getArg(2);
    llvm::IRBuilder<>& builder = gen.builder();
    for (size_t i = 0; i < 2; ++i) {
        llvm::Value* aElem = builder.CreateLoad(gen.getStructElemPtr(argA, i));
        llvm::Value* bElem = builder.CreateLoad(gen.getStructElemPtr(argB, i));
        llvm::Value* sum = builder.CreateFAdd(aElem, bElem);
        llvm::Value* resPartPtr = gen.getStructElemPtr(argRes, i);
        builder.CreateStore(sum, resPartPtr);
    }
}

} // unnamed namespace

void MathModule::registerTypes(Registry& registry) const {
    registry.registerType<ArgComplex>(
        [](llvm::LLVMContext& ctx) {
            llvm::Type* doubleTy = llvm::Type::getDoubleTy(ctx);
            return llvm::StructType::create(ctx, {doubleTy, doubleTy}, ArgComplex::name);
        },
        true);
}

void MathModule::registerFcts(Registry& registry) const {
    registry.registerFct(FctDesc<ArgComplex, ArgFloat, ArgFloat>(ArgComplex::name, complexCtor, complexCtorIntrinsic, FctFlags::Pure));
    registry.registerFct(FctDesc<ArgComplex, ArgComplex, ArgComplex>("operator_add", op<std::plus<>>, complexAddIntrinsic, FctFlags::Pure));
}

} // namespace jex
