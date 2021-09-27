#include <test_base.hpp>

#include <jex_environment.hpp>
#include <jex_errorhandling.hpp>
#include <jex_registry.hpp>

#include <gtest/gtest.h>

namespace jex {

namespace {

static constexpr char typeName[] = "UInt32";
using ArgUInt32 = Arg<uint32_t, typeName, jex::TypeKind::Value>;
static constexpr char typeNameBool[] = "Boolean";
using ArgBool = Arg<bool, typeNameBool, jex::TypeKind::Value>;

void pass(uint32_t* res, uint32_t in) {
    *res = in;
}
void add(uint32_t* res, uint32_t a, uint32_t b) {
    *res = a + b;
}
void passBool(bool* res, bool in) {} // LCOV_EXCL_LINE

}

TEST(Registry, registerType) {
    Environment env;
    Registry registry(env);
    registry.registerType<ArgUInt32>();
    ASSERT_EQ("UInt32", env.types().getType("UInt32")->name());
}

TEST(Registry, registerDuplicateType) {
    Environment env;
    Registry registry(env);


    registry.registerType<ArgUInt32>();
    ASSERT_THROW(registry.registerType<ArgUInt32>(), InternalError);
}

TEST(Registry, registerFct) {
    Environment env;
    Registry registry(env);
    // register types
    registry.registerType<ArgUInt32>();
    registry.registerType<ArgBool>();
    // register function
    registry.registerFct(FctDesc<ArgUInt32, ArgUInt32>("pass", pass));
    // register with different name
    registry.registerFct(FctDesc<ArgUInt32, ArgUInt32>("pass2", pass));
    // register with duplicate name
    ASSERT_THROW(registry.registerFct(FctDesc<ArgUInt32, ArgUInt32>("pass", pass)), InternalError);
    // register different function with same name (overload)
    registry.registerFct(FctDesc<ArgBool, ArgBool>("pass", passBool));
}

TEST(Registry, registerFctUnregisteredType) {
    Environment env;
    Registry registry(env);
    // ArgUInt32 not registered.
    ASSERT_THROW(registry.registerFct(FctDesc<ArgUInt32, ArgUInt32>("pass", pass)), InternalError);
}

TEST(FctLibrary, getFctUnregistered) {
    Environment env;
    Registry registry(env);
    registry.registerType<ArgUInt32>();
    TypeInfoId typeUInt32 = env.types().getType("UInt32");
    ASSERT_THROW(env.fctLib().getFct("pass", {typeUInt32}), InternalError);
}

TEST(FctLibrary, getFctSignatureChecks) {
    Environment env;
    test::registerBuiltIns(env);
    TypeSystem& typeSystem = env.types();
    FctLibrary& fctLibrary = env.fctLib();
    Registry registry(env);
    registry.registerType<ArgUInt32>();
    registry.registerType<ArgBool>();
    registry.registerFct(FctDesc<ArgUInt32, ArgUInt32>("pass", pass));

    TypeInfoId typeUInt32 = typeSystem.getType("UInt32");
    TypeInfoId typeBool = typeSystem.getType("Bool");
    // too many args
    ASSERT_THROW(fctLibrary.getFct("pass", {typeUInt32, typeUInt32}), InternalError);
    // too few args
    ASSERT_THROW(fctLibrary.getFct("pass", {}), InternalError);
    // wrong arg type
    ASSERT_THROW(fctLibrary.getFct("pass", {typeBool}), InternalError);
    // matching signature
    const jex::FctInfo& fct = fctLibrary.getFct("pass", {typeUInt32});
    ASSERT_EQ("pass", fct.d_name);
    ASSERT_EQ(reinterpret_cast<void*>(pass), fct.d_fctPtr);
    ASSERT_EQ(typeUInt32, fct.d_retType);
    ASSERT_EQ(std::vector{typeUInt32}, fct.d_paramTypes);
}

TEST(Registry, wrapperSimple) {
    uint32_t res = 0;
    uint32_t in = 42;
    void *args[] = {&res, &in};
    FctDesc<ArgUInt32, ArgUInt32>::wrapper(reinterpret_cast<void*>(pass), args);
    ASSERT_EQ(42, res);
}

TEST(Registry, wrapperMultipleArgs) {
    uint32_t res = 0;
    uint32_t a = 123;
    uint32_t b = 321;
    void *args[] = {&res, &a, &b};
    FctDesc<ArgUInt32, ArgUInt32, ArgUInt32>::wrapper(reinterpret_cast<void*>(add), args);
    ASSERT_EQ(444, res);
}

} // namespace jex
