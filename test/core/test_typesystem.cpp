#include <jex_compileenv.hpp>
#include <jex_errorhandling.hpp>
#include <jex_typesystem.hpp>

#include <gtest/gtest.h>

namespace jex {

TEST(TypeInfo, idEquals) {
    TypeInfo a(TypeId::Bool, "Bool", 1, nullptr);
    TypeInfo b(TypeId::Bool, "Bool", 1, nullptr);
    // Equality is based on memory address.
    EXPECT_FALSE(a.id() == b.id());
    EXPECT_TRUE(a.id() != b.id());
    EXPECT_TRUE(a.id() == a.id());
    EXPECT_FALSE(a.id() != a.id());
}

TEST(TypeInfo, getTypeInfoFromId) {
    TypeInfo ti(TypeId::Bool, "Bool", 1, nullptr);
    EXPECT_EQ(&ti, &ti.id().get());
    EXPECT_EQ("Bool", ti.id()->name());
}

TEST(TypeSystem, getUnregisteredType) {
    TypeSystem ts;
    EXPECT_THROW(ts.getType("Unregistered"), InternalError);
    EXPECT_EQ("_Unresolved", ts.getTypeOrUnresolved("Unregistered").get().name());
    EXPECT_EQ(ts.unresolved(), ts.getTypeOrUnresolved("Unregistered"));
}

TEST(TypeSystem, repeatedGetType) {
    TypeSystem ts;
    ts.registerType(jex::TypeId::Integer, "Integer", 8);
    TypeInfoId id1 = ts.getType("Integer");
    TypeInfoId id2 = ts.getType("Integer");
    EXPECT_TRUE(id1 == id2);
    EXPECT_FALSE(id1 != id2);
}

TEST(TypeSystem, getUnresolved) {
    TypeSystem ts;
    TypeInfoId invalid = ts.getTypeOrUnresolved("invalid");
    EXPECT_FALSE(ts.isResolved(invalid));
    EXPECT_EQ(TypeId::Unresolved, invalid->kind());
    EXPECT_EQ("_Unresolved", invalid->name());
}

} // namespace jex
