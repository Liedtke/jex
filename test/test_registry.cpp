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

} // namespace jex
