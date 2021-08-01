#include <jex_compileenv.hpp>
#include <jex_errorhandling.hpp>
#include <jex_registry.hpp>

#include <gtest/gtest.h>

namespace jex {

namespace {

static constexpr char typeName[] = "UInt32";
using ArgUInt32 = Arg<uint32_t, typeName, jex::TypeId::Complex>;
static constexpr char typeNameBool[] = "Boolean";
using ArgBool = Arg<bool, typeNameBool, jex::TypeId::Complex>;

void pass(uint32_t* res, uint32_t in) {} // LCOV_EXCL_LINE
void passBool(bool* res, bool in) {} // LCOV_EXCL_LINE

}

TEST(Registry, registerType) {
    CompileEnv env;
    Registry registry(env.typeSystem(), env.fctLibrary());
    registry.registerType<ArgUInt32>();
    ASSERT_EQ("UInt32", env.typeSystem().getType("UInt32")->name());
}

TEST(Registry, registerDuplicateType) {
    CompileEnv env;
    Registry registry(env.typeSystem(), env.fctLibrary());


    registry.registerType<ArgUInt32>();
    ASSERT_THROW(registry.registerType<ArgUInt32>(), InternalError);
}

TEST(Registry, registerFct) {
    CompileEnv env;
    Registry registry(env.typeSystem(), env.fctLibrary());
    // register types
    registry.registerType<ArgUInt32>();
    registry.registerType<ArgBool>();
    // register function
    registry.registerFct(FctDesc<ArgUInt32, ArgUInt32>(pass, "pass"));
    // register with different name
    registry.registerFct(FctDesc<ArgUInt32, ArgUInt32>(pass, "pass2"));
    // register with duplicate name
    ASSERT_THROW(registry.registerFct(FctDesc<ArgUInt32, ArgUInt32>(pass, "pass")), InternalError);
    // register different function with same name (overload)
    registry.registerFct(FctDesc<ArgBool, ArgBool>(passBool, "pass"));
}

TEST(Registry, registerFctUnregisteredType) {
    CompileEnv env;
    Registry registry(env.typeSystem(), env.fctLibrary());
    // ArgUInt32 not registered.
    ASSERT_THROW(registry.registerFct(FctDesc<ArgUInt32, ArgUInt32>(pass, "pass")), InternalError);
}

TEST(FctLibrary, getFctUnregistered) {
    CompileEnv env;
    Registry registry(env.typeSystem(), env.fctLibrary());
    registry.registerType<ArgUInt32>();
    TypeInfoId typeUInt32 = env.typeSystem().getType("UInt32");
    ASSERT_THROW(env.fctLibrary().getFct("pass", {typeUInt32}), InternalError);
}

TEST(FctLibrary, getFctSignatureChecks) {
    CompileEnv env;
    TypeSystem& typeSystem = env.typeSystem();
    FctLibrary& fctLibrary = env.fctLibrary();
    Registry registry(typeSystem, fctLibrary);
    registry.registerType<ArgUInt32>();
    registry.registerType<ArgBool>();
    registry.registerFct(FctDesc<ArgUInt32, ArgUInt32>(pass, "pass"));

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

} // namespace jex
