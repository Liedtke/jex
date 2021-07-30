#include <jex_compileenv.hpp>
#include <jex_errorhandling.hpp>
#include <jex_registry.hpp>

#include <gtest/gtest.h>

namespace jex {

TEST(Registry, registerType) {
    CompileEnv env;
    Registry registry(env.typeSystem(), env.fctLibrary());

    static constexpr char typeName[] = "UInt32";
    using ArgUInt32 = Arg<uint32_t, typeName, jex::TypeId::Complex>;
    registry.registerType<ArgUInt32>();
    ASSERT_EQ("UInt32", env.typeSystem().getType("UInt32")->name());
}

TEST(Registry, registerDuplicateType) {
    CompileEnv env;
    Registry registry(env.typeSystem(), env.fctLibrary());

    static constexpr char typeName[] = "UInt32";
    using ArgUInt32 = Arg<uint32_t, typeName, jex::TypeId::Complex>;
    registry.registerType<ArgUInt32>();
    ASSERT_THROW(registry.registerType<ArgUInt32>(), InternalError);
}

TEST(Registry, registerFct) {
    CompileEnv env;
    Registry registry(env.typeSystem(), env.fctLibrary());
    // add some type
    static constexpr char typeName[] = "UInt32";
    using ArgUInt32 = Arg<uint32_t, typeName, jex::TypeId::Complex>;
    registry.registerType<ArgUInt32>();
    // add another type
    static constexpr char typeNameBool[] = "Boolean";
    using ArgBool = Arg<bool, typeNameBool, jex::TypeId::Complex>;
    registry.registerType<ArgBool>();
    // register function
    auto pass = [](uint32_t* res, uint32_t in) { *res = in; };
    registry.registerFct(FctDesc<ArgUInt32, ArgUInt32>(pass, "pass"));
    // register with different name
    registry.registerFct(FctDesc<ArgUInt32, ArgUInt32>(pass, "pass2"));
    // register with duplicate name
    ASSERT_THROW(registry.registerFct(FctDesc<ArgUInt32, ArgUInt32>(pass, "pass")), InternalError);
    // register different function with same name (overload)
    auto passBool = [](bool* res, bool in) { *res = in; };
    registry.registerFct(FctDesc<ArgBool, ArgBool>(passBool, "pass"));
}

} // namespace jex
