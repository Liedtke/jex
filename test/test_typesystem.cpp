#include <jex_compileenv.hpp>
#include <jex_errorhandling.hpp>
#include <jex_typesystem.hpp>

#include <gtest/gtest.h>

namespace jex {

TEST(TypeInfo, idEquals) {
    TypeInfo a(TypeId::Bool, "Bool");
    TypeInfo b(TypeId::Bool, "Bool");
    // Equality is based on memory address.
    EXPECT_FALSE(a.id() == b.id());
    EXPECT_TRUE(a.id() != b.id());
    EXPECT_TRUE(a.id() == a.id());
    EXPECT_FALSE(a.id() != a.id());
}

TEST(TypeInfo, getTypeInfoFromId) {
    TypeInfo ti(TypeId::Bool, "Bool");
    EXPECT_EQ(&ti, &ti.id().get());
    EXPECT_EQ("Bool", ti.id()->name());
}

TEST(TypeSystem, getBuiltIns) {
    CompileEnv env;
    TypeSystem& ts = env.typeSystem();
    EXPECT_EQ("Integer", ts.getType("Integer").get().name());
    EXPECT_EQ("Float", ts.getType("Float").get().name());
    EXPECT_EQ("Bool", ts.getType("Bool").get().name());
    EXPECT_EQ("String", ts.getType("String").get().name());
}

TEST(TypeSystem, getUnregisteredType) {
    CompileEnv env;
    TypeSystem& ts = env.typeSystem();
    EXPECT_THROW(ts.getType("Unregistered"), InternalError);
    EXPECT_EQ("_Unresolved", ts.getTypeOrUnresolved("Unregistered").get().name());
    EXPECT_EQ(ts.unresolved(), ts.getTypeOrUnresolved("Unregistered"));
}

TEST(TypeSystem, repeatedGetType) {
    CompileEnv env;
    TypeSystem& ts = env.typeSystem();
    TypeInfoId id1 = ts.getType("Integer");
    TypeInfoId id2 = ts.getType("Integer");
    EXPECT_TRUE(id1 == id2);
    EXPECT_FALSE(id1 != id2);
}

} // namespace jex
