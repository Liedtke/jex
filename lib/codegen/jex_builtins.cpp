#include <jex_builtins.hpp>

#include "llvm/IR/DerivedTypes.h"

#include <cassert>

namespace jex {

namespace {

template <typename T>
void add(T* res, T a, T b) {
    assert(res != nullptr);
    *res = a + b;
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

} // anonymous namespace

void BuiltInsModule::registerTypes(Registry& registry) const {
    registry.registerType<ArgBool>(createBoolType);
    registry.registerType<ArgInteger>(createIntegerType);
    registry.registerType<ArgFloat>(createFloatType);
}

void BuiltInsModule::registerFcts(Registry& registry) const {
    registry.registerFct(FctDesc<ArgInteger, ArgInteger, ArgInteger>(add, "operator_add"));
    registry.registerFct(FctDesc<ArgFloat, ArgFloat, ArgFloat>(add, "operator_add"));
}

} // namespace jex
