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
using ArgUInt32 = ArgValue<uint32_t, typeName>;
static constexpr char typeNameInteger[] = "Integer";
using ArgInteger = ArgValue<int64_t, typeNameInteger>;
void pass(uint32_t* res, uint32_t in) {} // LCOV_EXCL_LINE
void add(uint32_t* res, uint32_t a, uint32_t b) {} // LCOV_EXCL_LINE

} // anonymous namespace

template <typename ParamT>
class TestTypeInferenceBase : public ::testing::TestWithParam<ParamT> {
protected:
    Environment d_env;
    std::unique_ptr<CompileEnv> d_compileEnv;

public:
    TestTypeInferenceBase() {
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

// === Error test cases === //

using TestErrorT = std::pair<const char* /*source*/, std::vector<const char*> /*errors*/>;
class TestTypeError : public TestTypeInferenceBase<TestErrorT> {};

TEST_P(TestTypeError, test) {
    Parser parser(*d_compileEnv, GetParam().first);
    parser.parse();
    TypeInference typeInference(*d_compileEnv);
    bool caught = false;
    try {
        typeInference.run();
    } catch (const CompileError&) {
        caught = true;
    }
    ASSERT_TRUE(caught);
    const auto& expMsgs = GetParam().second;
    const auto& actMsgs = d_compileEnv->messages();
    EXPECT_EQ(expMsgs.size(), actMsgs.size());
    auto expIter = expMsgs.begin();
    auto actIter = actMsgs.begin();
    for (; expIter != expMsgs.end() && actIter != actMsgs.end(); ++expIter, ++actIter) {
        std::stringstream err;
        err << *actIter;
        EXPECT_EQ(*expIter, err.str());
    }
}

static TestErrorT errorTests[] = {
    {"var a: UInt32 = pass();", {"1.17-1.22: Error: No matching candidate found for function 'pass()'"}},
    {"var a: UInt32 = 1;", {"1.1-1.17: Error: Invalid type for variable 'a': Specified as 'UInt32' but expression returns 'Integer'"}},
    {"var a: UInt32 = x + 1;", {"1.17-1.21: Error: No matching candidate found for function 'operator_add(UInt32, Integer)'"}},
    // Only the inner resolve errors are reported as the outer ones are only follow-up errors.
    {"var a: UInt32 = add(pass(pass()), add());", {
        "1.26-1.31: Error: No matching candidate found for function 'pass()'",
        "1.35-1.39: Error: No matching candidate found for function 'add()'",
    }},
    {"var a: UInt32 = x + 1 + (1 + x);", {
        "1.17-1.21: Error: No matching candidate found for function 'operator_add(UInt32, Integer)'",
        "1.26-1.30: Error: No matching candidate found for function 'operator_add(Integer, UInt32)'",
    }},
    {"var a: UInt32 = if(x, x, x);",
        {"1.17-1.26: Error: 'if' function requires first argument to be of type 'Bool', 'UInt32' given"}},
    {"var a: UInt32 = if(true, x, true);",
        {"1.17-1.32: Error: 'if' function requires second and third argument to have the same type, 'UInt32' and 'Bool' given"}},
    {"var a: Bool = if(true, x, x+1);",
        // No further errors are reported as the inner call is unresolved.
        {"1.27-1.29: Error: No matching candidate found for function 'operator_add(UInt32, Integer)'"}},
    {"var a: Integer = if(true, x, x);",
        {"1.1-1.30: Error: Invalid type for variable 'a': Specified as 'Integer' but expression returns 'UInt32'"}},
    {"var a: Integer = if(true, x, x, x);",
        {"1.18-1.33: Error: 'if' function requires exactly 3 arguments, 4 given"}},
};

INSTANTIATE_TEST_SUITE_P(SuiteTypeInferenceError,
                         TestTypeError,
                         testing::ValuesIn(errorTests));

// === Success test cases === //

class TestSuccess : public TestTypeInferenceBase<const char*> {};

TEST_P(TestSuccess, test) {
    Parser parser(*d_compileEnv, GetParam());
    parser.parse();
    TypeInference typeInference(*d_compileEnv);
    typeInference.run();
    ASSERT_FALSE(d_compileEnv->hasErrors());
}

static const char* successTests[] = {
    "var a: UInt32 = pass(x);", // resolve function
    "var a: UInt32 = x + x;", // resolve operator
    "var a: UInt32 = x + x - x * x / x % x;", // resolve operators nested arithmetics
    "var a: UInt32 = x == x != x < x <= x > x >= x;", // resolve operators nested comparisons
    "var a: UInt32 = if(true, x, x+x);",
    "var a: UInt32 = if(true, if(false, x*x, x+x), x);",
};

INSTANTIATE_TEST_SUITE_P(SuiteTypeInference,
                         TestSuccess,
                         testing::ValuesIn(successTests));

// === Operator names test cases === //

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
