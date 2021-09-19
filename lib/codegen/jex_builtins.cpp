#include <jex_builtins.hpp>
#include <jex_intrinsicgen.hpp>

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"

#include <cassert>
#include <functional>

namespace jex {

namespace {

template <typename T>
void add(T* res, T a, T b) {
    assert(res != nullptr);
    *res = a + b;
}

template <typename T>
void sub(T* res, T a, T b) {
    assert(res != nullptr);
    *res = a - b;
}

template <typename T>
void mul(T* res, T a, T b) {
    assert(res != nullptr);
    *res = a * b;
}

template <typename T>
void div(T* res, T a, T b) {
    assert(res != nullptr);
    assert(b != 0);
    *res = a / b;
}

template <typename T>
void mod(T* res, T a, T b) {
    assert(res != nullptr);
    assert(b != 0);
    *res = a % b;
}

template <typename T>
void equal(bool* res, T a, T b) {
    assert(res != nullptr);
    *res = a == b;
}

template <typename T>
void notEqual(bool* res, T a, T b) {
    assert(res != nullptr);
    *res = a != b;
}

template <typename T>
void less(bool* res, T a, T b) {
    assert(res != nullptr);
    *res = a < b;
}

template <typename T>
void greater(bool* res, T a, T b) {
    assert(res != nullptr);
    *res = a > b;
}

template <typename T>
void lessEqual(bool* res, T a, T b) {
    assert(res != nullptr);
    *res = a <= b;
}

template <typename T>
void greaterEqual(bool* res, T a, T b) {
    assert(res != nullptr);
    *res = a >= b;
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
    // === Bool ===
    // Comparisons
    registry.registerFct(FctDesc<ArgBool, ArgBool, ArgBool>("operator_eq", equal));
    registry.registerFct(FctDesc<ArgBool, ArgBool, ArgBool>("operator_ne", notEqual));

    // === Integer ===
    // Arithmetics
    registry.registerFct(FctDesc<ArgInteger, ArgInteger, ArgInteger>("operator_add", add, generateIntegerAdd));
    registry.registerFct(FctDesc<ArgInteger, ArgInteger, ArgInteger>("operator_sub", sub));
    registry.registerFct(FctDesc<ArgInteger, ArgInteger, ArgInteger>("operator_mul", mul));
    registry.registerFct(FctDesc<ArgInteger, ArgInteger, ArgInteger>("operator_div", div));
    registry.registerFct(FctDesc<ArgInteger, ArgInteger, ArgInteger>("operator_mod", mod));
    // Comparisons
    registry.registerFct(FctDesc<ArgBool, ArgInteger, ArgInteger>("operator_eq", equal));
    registry.registerFct(FctDesc<ArgBool, ArgInteger, ArgInteger>("operator_ne", notEqual));
    registry.registerFct(FctDesc<ArgBool, ArgInteger, ArgInteger>("operator_lt", less));
    registry.registerFct(FctDesc<ArgBool, ArgInteger, ArgInteger>("operator_gt", greater));
    registry.registerFct(FctDesc<ArgBool, ArgInteger, ArgInteger>("operator_le", lessEqual));
    registry.registerFct(FctDesc<ArgBool, ArgInteger, ArgInteger>("operator_ge", greaterEqual));

    // === Float ===
    // Arithmetics
    registry.registerFct(FctDesc<ArgFloat, ArgFloat, ArgFloat>("operator_add", add));
    registry.registerFct(FctDesc<ArgFloat, ArgFloat, ArgFloat>("operator_sub", sub));
    registry.registerFct(FctDesc<ArgFloat, ArgFloat, ArgFloat>("operator_mul", mul));
    registry.registerFct(FctDesc<ArgFloat, ArgFloat, ArgFloat>("operator_div", div));
    // Comparisons
    registry.registerFct(FctDesc<ArgBool, ArgFloat, ArgFloat>("operator_eq", equal));
    registry.registerFct(FctDesc<ArgBool, ArgFloat, ArgFloat>("operator_ne", notEqual));
    registry.registerFct(FctDesc<ArgBool, ArgFloat, ArgFloat>("operator_lt", less));
    registry.registerFct(FctDesc<ArgBool, ArgFloat, ArgFloat>("operator_gt", greater));
    registry.registerFct(FctDesc<ArgBool, ArgFloat, ArgFloat>("operator_le", lessEqual));
    registry.registerFct(FctDesc<ArgBool, ArgFloat, ArgFloat>("operator_ge", greaterEqual));
}

} // namespace jex
