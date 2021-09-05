#include <jex_builtins.hpp>
#include <jex_intrinsicgen.hpp>

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"

#include <cassert>

namespace jex {

namespace {

template <typename T>
void add(T* res, T a, T b) {
    assert(res != nullptr);
    *res = a + b;
}

template <typename T>
void mul(T* res, T a, T b) {
    assert(res != nullptr);
    *res = a * b;
}

llvm::Type* createBoolType(llvm::LLVMContext& ctx) {
    return llvm::Type::getInt1Ty(ctx);
}
llvm::Type* createIntegerType(llvm::LLVMContext& ctx) {
    return llvm::Type::getInt64Ty(ctx);
}
llvm::Type* createFloatType(llvm::LLVMContext& ctx) {
    return llvm::Type::getDoubleTy(ctx);
}

void generateIntegerAdd(IntrinsicGen& gen) {
    llvm::IRBuilder<>& builder = gen.builder();
    llvm::Value* result = builder.CreateAdd(gen.fct().getArg(1), gen.fct().getArg(2), "result");
    builder.CreateStore(result, gen.fct().getArg(0));
}

} // anonymous namespace

void BuiltInsModule::registerTypes(Registry& registry) const {
    registry.registerType<ArgBool>(createBoolType);
    registry.registerType<ArgInteger>(createIntegerType);
    registry.registerType<ArgFloat>(createFloatType);
}

void BuiltInsModule::registerFcts(Registry& registry) const {
    registry.registerFct(FctDesc<ArgInteger, ArgInteger, ArgInteger>("operator_add", add, generateIntegerAdd));
    registry.registerFct(FctDesc<ArgFloat, ArgFloat, ArgFloat>("operator_add", add));

    registry.registerFct(FctDesc<ArgInteger, ArgInteger, ArgInteger>("operator_mul", mul));
    registry.registerFct(FctDesc<ArgFloat, ArgFloat, ArgFloat>("operator_mul", mul));
}

} // namespace jex
