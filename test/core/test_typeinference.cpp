#include <test_base.hpp>

#include <jex_ast.hpp>
#include <jex_compileenv.hpp>
#include <jex_errorhandling.hpp>
#include <jex_parser.hpp>
#include <jex_registry.hpp>
#include <jex_symboltable.hpp>
#include <jex_typeinference.hpp>

#include <gtest/gtest.h>

#include <memory>
#include <sstream>

namespace jex {

namespace {

static constexpr char typeName[] = "UInt32";
using ArgUInt32 = Arg<uint32_t, typeName, jex::TypeId::Complex>;
static constexpr char typeNameInteger[] = "Integer";
using ArgInteger = Arg<int64_t, typeNameInteger, jex::TypeId::Integer>;
void pass(uint32_t* res, uint32_t in) {} // LCOV_EXCL_LINE
void add(uint32_t* res, uint32_t a, uint32_t b) {} // LCOV_EXCL_LINE

} // anonymous namespace

class TestTypeInference : public ::testing::Test {
protected:
    Environment d_env;
    std::unique_ptr<CompileEnv> d_compileEnv;

public:
    TestTypeInference() {
        test::registerBuiltIns(d_env);
        Registry registry(d_env);
        registry.registerType<ArgUInt32>();
        registry.registerFct(FctDesc<ArgUInt32, ArgUInt32>("pass", pass));
        registry.registerFct(FctDesc<ArgUInt32, ArgUInt32, ArgUInt32>("add", add));
        registry.registerFct(FctDesc<ArgUInt32, ArgUInt32, ArgUInt32>("operator_add", add));
        registry.registerFct(FctDesc<ArgUInt32, ArgUInt32, ArgUInt32>("operator_sub", add));
        registry.registerFct(FctDesc<ArgUInt32, ArgUInt32, ArgUInt32>("operator_mul", add));
        registry.registerFct(FctDesc<ArgUInt32, ArgUInt32, ArgUInt32>("operator_div", add));
        registry.registerFct(FctDesc<ArgUInt32, ArgUInt32, ArgUInt32>("operator_mod", add));
        // For these tests, the comparison returns UInt32, although it should certainly be bool.
        registry.registerFct(FctDesc<ArgUInt32, ArgUInt32, ArgUInt32>("operator_eq", add));
        registry.registerFct(FctDesc<ArgUInt32, ArgUInt32, ArgUInt32>("operator_ne", add));
        registry.registerFct(FctDesc<ArgUInt32, ArgUInt32, ArgUInt32>("operator_lt", add));
        registry.registerFct(FctDesc<ArgUInt32, ArgUInt32, ArgUInt32>("operator_gt", add));
        registry.registerFct(FctDesc<ArgUInt32, ArgUInt32, ArgUInt32>("operator_le", add));
        registry.registerFct(FctDesc<ArgUInt32, ArgUInt32, ArgUInt32>("operator_ge", add));
    }

    void SetUp() override {
        d_compileEnv = std::make_unique<CompileEnv>(d_env);
        // TODO: Add support for proper type inference of literals.
        d_compileEnv->symbols().addSymbol(Location(), Symbol::Kind::Variable, "x", d_env.types().getType("UInt32"));
    }
};

TEST_F(TestTypeInference, resolveFct) {
    Parser parser(*d_compileEnv, "var a: UInt32 = pass(x);");
    parser.parse();
    TypeInference typeInference(*d_compileEnv);
    typeInference.run();
    ASSERT_FALSE(d_compileEnv->hasErrors());
}

TEST_F(TestTypeInference, resolveFctInvalidOverload) {
    Parser parser(*d_compileEnv, "var a: UInt32 = pass();");
    parser.parse();
    TypeInference typeInference(*d_compileEnv);
    // TODO: Should the type inference throw in case of errors?
    try {
        typeInference.run();
        ASSERT_TRUE(false); // LCOV_EXCL_LINE
    } catch (const CompileError&) {
        ASSERT_EQ(1, d_compileEnv->messages().size());
        std::stringstream err;
        err << *d_compileEnv->messages().begin();
        ASSERT_EQ("1.17-1.22: Error: No matching candidate found for function 'pass()'", err.str());
    }
}

TEST_F(TestTypeInference, resolveFctInvalidOverloadRepeated) {
    Parser parser(*d_compileEnv, "var a: UInt32 = add(pass(pass()), add());");
    parser.parse();
    TypeInference typeInference(*d_compileEnv);
    try {
        typeInference.run();
        ASSERT_TRUE(false); // LCOV_EXCL_LINE
    } catch (const CompileError&) {
        // Only the inner errors gets reported as the outer ones are only follow-up errors.
        ASSERT_EQ(2, d_compileEnv->messages().size());
        std::stringstream err;
        auto iter = d_compileEnv->messages().begin();
        err << *iter << '\n';
        ++iter;
        err << *iter;
        ASSERT_EQ(
            "1.26-1.31: Error: No matching candidate found for function 'pass()'\n"
            "1.35-1.39: Error: No matching candidate found for function 'add()'",
            err.str());
    }
}

TEST_F(TestTypeInference, resolveOperator) {
    Parser parser(*d_compileEnv, "var a: UInt32 = x + x;");
    parser.parse();
    TypeInference typeInference(*d_compileEnv);
    typeInference.run();
    ASSERT_FALSE(d_compileEnv->hasErrors());
}

TEST_F(TestTypeInference, resolveWrongVarType) {
    Parser parser(*d_compileEnv, "var a: UInt32 = 1;");
    parser.parse();
    TypeInference typeInference(*d_compileEnv);
    ASSERT_THROW(typeInference.run(), CompileError);
}

TEST_F(TestTypeInference, resolveOperatorArithmetic) {
    Parser parser(*d_compileEnv, "var a: UInt32 = x + x - x * x / x % x;");
    parser.parse();
    TypeInference typeInference(*d_compileEnv);
    typeInference.run();
    ASSERT_FALSE(d_compileEnv->hasErrors());
}

TEST_F(TestTypeInference, resolveOperatorComparison) {
    Parser parser(*d_compileEnv, "var a: UInt32 = x == x != x < x <= x > x >= x;");
    parser.parse();
    TypeInference typeInference(*d_compileEnv);
    typeInference.run();
    ASSERT_FALSE(d_compileEnv->hasErrors());
}

TEST_F(TestTypeInference, resolveOperatorInvalidOverload) {
    Parser parser(*d_compileEnv, "var a: UInt32 = x + 1;");
    parser.parse();
    TypeInference typeInference(*d_compileEnv);
    try {
        typeInference.run();
        ASSERT_TRUE(false); // LCOV_EXCL_LINE
    } catch (const CompileError&) {
        ASSERT_EQ(1, d_compileEnv->messages().size());
        std::stringstream err;
        err << *d_compileEnv->messages().begin();
        ASSERT_EQ("1.17-1.21: Error: No matching candidate found for function 'operator_add(UInt32, Integer)'", err.str());
    }
}

TEST_F(TestTypeInference, resolveOperatorInvalidOverloadRepeated) {
    Parser parser(*d_compileEnv, "var a: UInt32 = x + 1 + (1 + x);");
    parser.parse();
    TypeInference typeInference(*d_compileEnv);
    try {
        typeInference.run();
        ASSERT_TRUE(false); // LCOV_EXCL_LINE
    } catch (const CompileError&) {
        // Only the inner errors gets reported as the outer ones are only follow-up errors.
        ASSERT_EQ(2, d_compileEnv->messages().size());
        std::stringstream err;
        auto iter = d_compileEnv->messages().begin();
        err << *iter << '\n';
        ++iter;
        err << *iter;
        ASSERT_EQ(
            "1.17-1.21: Error: No matching candidate found for function 'operator_add(UInt32, Integer)'\n"
            "1.26-1.30: Error: No matching candidate found for function 'operator_add(Integer, UInt32)'",
            err.str());
    }
}

using TestOpNameExp = std::pair<const char*, const char*>;

class TestOpName : public testing::TestWithParam<TestOpNameExp> {};

TEST_P(TestOpName, test) {
    Environment env;
    Registry registry(env);
    registry.registerType<ArgInteger>();
    CompileEnv compileEnv(env);
    std::string code = std::string("var a: Integer = 1 ") + GetParam().first + " 1;";
    Parser parser(compileEnv, code.c_str());
    parser.parse();
    TypeInference typeInference(compileEnv);
    try {
        typeInference.run();
    } catch (const CompileError&) {
        ASSERT_EQ(1, compileEnv.messages().size());
        std::stringstream err;
        err << *compileEnv.messages().begin();
        std::stringstream exp;
        exp << "1.18-1." << (21 + strlen(GetParam().first))
            << ": Error: Invalid function name '"
            << GetParam().second << "'";
        EXPECT_EQ(exp.str(), err.str());
    }
}

static TestOpNameExp opNameTests[] = {
    {"+", "operator_add"},
    {"-", "operator_sub"},
    {"*", "operator_mul"},
    {"/", "operator_div"},
    {"%", "operator_mod"},
    {"==", "operator_eq"},
    {"!=", "operator_ne"},
    {"<", "operator_lt"},
    {">", "operator_gt"},
    {"<=", "operator_le"},
    {">=", "operator_ge"},
};

INSTANTIATE_TEST_SUITE_P(SuiteOpName,
                         TestOpName,
                         testing::ValuesIn(opNameTests));

} // namespace jex
