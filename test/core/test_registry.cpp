#include <test_base.hpp>

#include <jex_environment.hpp>
#include <jex_errorhandling.hpp>
#include <jex_registry.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace jex {

namespace {

static constexpr char typeName[] = "UInt32";
using ArgUInt32 = ArgValue<uint32_t, typeName>;
static constexpr char typeNameBool[] = "Boolean";
using ArgBool = ArgValue<bool, typeNameBool>;

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

TEST(Registry, registerComplexType) {
    struct Counts {
        size_t dtor = 0;
        size_t copyCtor = 0;
        size_t moveCtor = 0;

        bool operator==(const Counts& other) const {
            return dtor == other.dtor && copyCtor == other.copyCtor && moveCtor == other.moveCtor;
        }
    };
    struct TestClass {
        Counts* counts;
        TestClass(Counts* counts) : counts(counts) {}
        TestClass(const TestClass& other) : counts(other.counts) {
            ++counts->copyCtor;
        }
        TestClass(TestClass&& other) : counts(other.counts) {
            ++counts->moveCtor;
        }
        ~TestClass() {
            ++counts->dtor;
        }
    };

    // Register type.
    static constexpr char name[] = "TestClass";
    using ArgTestClass = ArgObject<TestClass, name>;
    Environment env;
    Registry registry(env);
    registry.registerType<ArgTestClass>();
    // Check type info.
    const TypeInfo& type = *env.types().getType("TestClass");
    ASSERT_EQ("TestClass", type.name());
    ASSERT_EQ(TypeKind::Complex, type.kind());
    ASSERT_EQ(sizeof(TestClass), type.size());
    // Check lifetime functions.
    Counts counts;
    TestClass testObj(&counts);
    TestClass other(&counts);
    type.lifetimeFcts().destructor(&other);
    EXPECT_TRUE((Counts{1, 0, 0} == counts));
    type.lifetimeFcts().copyConstructor(&other, &testObj);
    EXPECT_TRUE((Counts{1, 1, 0} == counts));
    type.lifetimeFcts().destructor(&other);
    EXPECT_TRUE((Counts{2, 1, 0} == counts));
    type.lifetimeFcts().moveConstructor(&other, &testObj);
    EXPECT_TRUE((Counts{2, 1, 1} == counts));
}

} // namespace jex
