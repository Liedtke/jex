#include <test_base.hpp>

#include <jex_ast.hpp>
#include <jex_compileenv.hpp>
#include <jex_errorhandling.hpp>
#include <jex_parser.hpp>
#include <jex_prettyprinter.hpp>
#include <jex_registry.hpp>
#include <jex_symboltable.hpp>
#include <jex_typeinference.hpp>

#include <gtest/gtest.h>

#include <memory>
#include <sstream>

namespace jex {

namespace {

constexpr char typeName[] = "UInt32";
using ArgUInt32 = ArgValue<uint32_t, typeName>;
constexpr char typeNameInteger[] = "Integer";
using ArgInteger = ArgValue<int64_t, typeNameInteger>;
void pass(uint32_t* res, uint32_t in) {} // LCOV_EXCL_LINE
void add(uint32_t* res, uint32_t a, uint32_t b) {} // LCOV_EXCL_LINE
void max(uint32_t* res, const VarArg<uint32_t>* args) {} // LCOV_EXCL_LINE
void max(uint32_t* res, uint32_t a, uint32_t b) {} // LCOV_EXCL_LINE

void registerTestFcts(Environment& env) {
    Registry registry(env);
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
    // Add unary operators.
    registry.registerFct(FctDesc<ArgUInt32, ArgUInt32>("operator_uminus", pass));
    registry.registerFct(FctDesc<ArgUInt32, ArgUInt32>("operator_not", pass));
    // Same function without var arg for exactly 2 arguments.
    registry.registerFct(FctDesc<ArgUInt32, ArgUInt32, ArgUInt32>("max", max));
    // Function with var arg.
    registry.registerFct(FctDesc<ArgUInt32, ArgVarArg<ArgUInt32>>("max", max));
}

} // anonymous namespace

template <typename ParamT>
class TestTypeInferenceBase : public ::testing::TestWithParam<ParamT> {
protected:
    Environment d_env;
    std::unique_ptr<CompileEnv> d_compileEnv;

public:
    TestTypeInferenceBase() {
        test::registerBuiltIns(d_env);
        registerTestFcts(d_env);
    }

    void SetUp() override {
        d_compileEnv = std::make_unique<CompileEnv>(d_env);
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
    {"var a: UInt32 = pass();", {"1.17-1.22: Error: No matching candidate found for function 'pass()'. Candidates are:\n  UInt32 pass(UInt32)"}},
    {"var a: UInt32 = 1;", {"1.1-1.17: Error: Invalid type for variable 'a': Specified as 'UInt32' but expression returns 'Integer'"}},
    {"var a: UInt32 = x + 1;", {
R"(1.17-1.21: Error: No matching candidate found for function 'operator_add(UInt32, Integer)'. Candidates are:
  Integer operator_add(Integer, Integer)
  UInt32 operator_add(UInt32, UInt32))"}},
    // Only the inner resolve errors are reported as the outer ones are only follow-up errors.
    {"var a: UInt32 = add(pass(pass()), add());", {
        "1.26-1.31: Error: No matching candidate found for function 'pass()'. Candidates are:\n  UInt32 pass(UInt32)",
        "1.35-1.39: Error: No matching candidate found for function 'add()'. Candidates are:\n  UInt32 add(UInt32, UInt32)",
    }},
    {"var a: UInt32 = x + 1 + (1 + x);", {
R"(1.17-1.21: Error: No matching candidate found for function 'operator_add(UInt32, Integer)'. Candidates are:
  Integer operator_add(Integer, Integer)
  UInt32 operator_add(UInt32, UInt32))",
R"(1.26-1.30: Error: No matching candidate found for function 'operator_add(Integer, UInt32)'. Candidates are:
  Integer operator_add(Integer, Integer)
  UInt32 operator_add(UInt32, UInt32))",
    }},
    {"var a: UInt32 = if(x, x, x);",
        {"1.17-1.27: Error: 'if' function requires first argument to be of type 'Bool', 'UInt32' given"}},
    {"var a: UInt32 = if(true, x, true);",
        {"1.17-1.33: Error: 'if' function requires second and third argument to have the same type, 'UInt32' and 'Bool' given"}},
    // No further errors are reported as the inner call is unresolved.
    {"var a: Bool = if(true, x, x+1);", {
R"(1.27-1.29: Error: No matching candidate found for function 'operator_add(UInt32, Integer)'. Candidates are:
  Integer operator_add(Integer, Integer)
  UInt32 operator_add(UInt32, UInt32))"
    }},
    {"var a: Integer = if(true, x, x);",
        {"1.1-1.31: Error: Invalid type for variable 'a': Specified as 'Integer' but expression returns 'UInt32'"}},
    {"var a: Integer = if(true, x, x, x);",
        {"1.18-1.34: Error: 'if' function requires exactly 3 arguments, 4 given"}},
    // No further errors are reported as the inner call is unresolved.
    {"var a: Bool = -(x+1);", {
R"(1.17-1.19: Error: No matching candidate found for function 'operator_add(UInt32, Integer)'. Candidates are:
  Integer operator_add(Integer, Integer)
  UInt32 operator_add(UInt32, UInt32))"
    }},
    // Var Args: At minimum one argument required.
    {"var a: UInt32 = max();", {
R"(1.17-1.21: Error: No matching candidate found for function 'max()'. Candidates are:
  UInt32 max(UInt32, UInt32)
  UInt32 max(_VarArg<UInt32>))"
    }},
    {"var a: UInt32 = max(1);", {
R"(1.17-1.22: Error: No matching candidate found for function 'max(Integer)'. Candidates are:
  UInt32 max(UInt32, UInt32)
  UInt32 max(_VarArg<UInt32>))"
    }},
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
    "var a: UInt32 = -x;",
    "var a: UInt32 = --------x;",
    "var a: UInt32 = !x;",
    "var a: UInt32 = !!!!!!!!x;",
    "var a: UInt32 = max(x, x);", // special registered function
    "var a: UInt32 = max(x, x, x);", // var arg
    "var a: UInt32 = max(x);", // var arg
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
    {"&", "operator_bitand"},
    {"|", "operator_bitor"},
    {"^", "operator_bitxor"},
};

INSTANTIATE_TEST_SUITE_P(SuiteOpName,
                         TestOpName,
                         testing::ValuesIn(opNameTests));

TEST(TypeInference, varArgs) {
    Environment env;
    test::registerBuiltIns(env);
    registerTestFcts(env);
    CompileEnv compileEnv(env);
    compileEnv.symbols().addSymbol(Location(), Symbol::Kind::Variable, "x", env.types().getType("UInt32"));
    std::string code = std::string(R"(
var a: UInt32 = max(x, x+x); // Exactly two args --> no vararg
var b: UInt32 = max(x); // vararg
var c: UInt32 = max(x, x, x, x);
)");
    Parser parser(compileEnv, code.c_str());
    parser.parse();
    TypeInference typeInference(compileEnv);
    typeInference.run();
    std::stringstream out;
    PrettyPrinter printer(out);
    compileEnv.getRoot()->accept(printer);
    const char* expected =
R"(var a: UInt32 = max(x, (x + x));
var b: UInt32 = max([x]);
var c: UInt32 = max([x, x, x, x]);
)";
    ASSERT_EQ(expected, out.str());
}

} // namespace jex
