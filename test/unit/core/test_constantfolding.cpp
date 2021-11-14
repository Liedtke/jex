#include <test_base.hpp>

#include <jex_ast.hpp>
#include <jex_compileenv.hpp>
#include <jex_constantfolding.hpp>
#include <jex_environment.hpp>
#include <jex_errorhandling.hpp>
#include <jex_parser.hpp>
#include <jex_prettyprinter.hpp>
#include <jex_typeinference.hpp>

#include <gtest/gtest.h>

namespace jex {

using ConstantValue = std::variant<int64_t, std::string, bool>;
struct ConstantExp {
    const char* name;
    ConstantValue value;
};
struct TestConstFoldingT {
    const char* sourceCode;
    const char* expDump;
    std::vector<ConstantExp> expConstants;
    bool enableConstFolding = true;
};

class TestConstFolding : public testing::TestWithParam<TestConstFoldingT> {
};

TEST_P(TestConstFolding, test) {
    Environment env;
    test::registerBuiltIns(env);
    CompileEnv compileEnv(env, false);
    Parser parser(compileEnv, GetParam().sourceCode);
    parser.parse();
    TypeInference typeInference(compileEnv);
    typeInference.run();
    ConstantFolding constantFolding(compileEnv, GetParam().enableConstFolding);
    constantFolding.run();
    // Check pretty-printed AST.
    std::stringstream str;
    PrettyPrinter printer(str);
    compileEnv.getRoot()->accept(printer);
    ASSERT_EQ(GetParam().expDump, str.str());
    // Check constants.
    for (const ConstantExp& expConst : GetParam().expConstants) {
        const void* valPtr = compileEnv.constants().constantByName(expConst.name).getPtr();
        ASSERT_TRUE(valPtr != nullptr) << expConst.name;
        std::visit([&](auto val) {
            ASSERT_EQ(val, *reinterpret_cast<const decltype(val)*>(valPtr)) << expConst.name;
        }, expConst.value);
    }
}

constexpr int64_t operator"" _i64(unsigned long long in) {
    return static_cast<int64_t>(in);
}

static TestConstFoldingT tests[] = {
    {   // Test folding of simple expression.
        "expr x: Integer = 1 + 2 + 4 + (2 * 1) + (1 + 0);",
        "expr x: Integer = (([const_Integer_l1_c19] + (2 * 1)) + [const_Integer_l1_c42]);\n",
        {{"const_Integer_l1_c19", 7_i64}, {"const_Integer_l1_c42", 1_i64}}
    },
    {   // Test folding of if expression true branch.
        "expr x: Integer = if(true, (1+2)*3, 2+1);",
        "expr x: Integer = ([const_Integer_l1_c29] * 3);\n",
        {{"const_Integer_l1_c29", 3_i64}}
    },
    {   // Test folding of if expression false branch, continued folding.
        "expr x: Integer = if(false, 2*3, 1+2) + 3;",
        "expr x: Integer = [const_Integer_l1_c19];\n",
        {{"const_Integer_l1_c19", 6_i64}}
    },
    {   // Test folding of nested if.
        "expr x: Integer = if(false, 2*3, if(true, 1+2, 3+4) + 5) + 6;",
        "expr x: Integer = [const_Integer_l1_c19];\n",
        {{"const_Integer_l1_c19", 14_i64}}
    },
    {   // Test folding of nested if condition.
        "expr x: Integer = if(if(false, true, false), 2*3, (1+1)*2);",
        "expr x: Integer = ([const_Integer_l1_c52] * 2);\n",
        {{"const_Integer_l1_c52", 2_i64}}
    },
    {   // Test folding of if branches with non-const condition.
        "expr x: Integer = if(2*2 == 3, 1+1, 2+2);",
        "expr x: Integer = if(((2 * 2) == 3), [const_Integer_l1_c32], [const_Integer_l1_c37]);\n",
        {{"const_Integer_l1_c32", 2_i64}, {"const_Integer_l1_c37", 4_i64}}
    },
    {   // Test folding of complex type.
        "expr x: String = substr(\"test\", 2, 2);",
        "expr x: String = [const_String_l1_c18];\n",
        {{"const_String_l1_c18", std::string("st")}}
    },
    {   // Test folding of complex type nested.
        "expr x: String = substr(substr(\"This is a decently sized string for allocations\", 0, 100), 10, 6);",
        "expr x: String = [const_String_l1_c18];\n",
        {{"const_String_l1_c18", std::string("decent")}}
    },
    {   // Test folding with variable declared as const.
        "const x: String = substr(substr(\"This is a decently sized string for allocations\", 0, 100), 10, 6);",
        "const x: String = [const_String_l1_c19];\n",
        {{"const_String_l1_c19", std::string("decent")}}
    },
    {   // Test folding with variable declared as const and const folding disabled.
        // As the varialbe is declared as const, it still has to be folded.
        "const x: String = substr(substr(\"This is a decently sized string for allocations\", 0, 100), 10, 6);",
        "const x: String = [const_String_l1_c19];\n",
        {{"const_String_l1_c19", std::string("decent")}},
        false
    },
    {   // Test folding with const folding disabled.
        // As the varialbe is not declared as const, it may not be folded.
        "expr x: Integer = 1 + 1;",
        "expr x: Integer = (1 + 1);\n",
        {},
        false
    },
    {   // Unary-not is registered as foldable.
        "expr x: Bool = !true;",
        "expr x: Bool = [const_Bool_l1_c16];\n",
        {{"const_Bool_l1_c16", false}},
    },
    {   // Nesting of unary not.
        "expr x: Bool = !!!!true;",
        "expr x: Bool = [const_Bool_l1_c16];\n",
        {{"const_Bool_l1_c16", true}},
    },
    {   // Unary-minus is not registered as foldable.
        "expr x: Integer = -(1 + 3);",
        "expr x: Integer = -[const_Integer_l1_c21];\n",
        {{"const_Integer_l1_c21", 4_i64}},
    },
    {   // true && ... --> ...
        "expr x: Bool = getConst(true) && getNonConst(false);",
        "expr x: Bool = getNonConst(false);\n",
    },
    {   // false && ... --> false
        "expr x: Bool = false && getNonConst(true);",
        "expr x: Bool = false;\n",
    },
    {   // false && ... --> false
        "expr x: Bool = getConst(false) && getConst(true);",
        "expr x: Bool = [const_Bool_l1_c16];\n",
        {{"const_Bool_l1_c16", false}},
    },
    {   // If && lhs can't be evaluated, rhs is not folded at all.
        "expr x: Bool = getNonConst(false) && getConst(true);",
        "expr x: Bool = (getNonConst(false) && getConst(true));\n",
    },
    {   // false || ... --> ...
        "expr x: Bool = getConst(false) || getNonConst(false);",
        "expr x: Bool = getNonConst(false);\n",
    },
    {   // true || ... --> true
        "expr x: Bool = getConst(true) || getNonConst(false);",
        "expr x: Bool = [const_Bool_l1_c16];\n",
        {{"const_Bool_l1_c16", true}},
    },
    {   // true || ... --> true
        "expr x: Bool = true || getNonConst(false);",
        "expr x: Bool = true;\n",
    },
    {   // If || lhs can't be evaluated, rhs is not folded at all.
        "expr x: Bool = getNonConst(true) || getConst(false);",
        "expr x: Bool = (getNonConst(true) || getConst(false));\n",
    },
    {   // Constants used in other constants are also folded.
        "const x: Integer = 1 + 1; const y: Integer = 3; const z : Integer = x + x + y;",
        "const x: Integer = [const_Integer_l1_c20];\nconst y: Integer = 3;\nconst z: Integer = [const_Integer_l1_c69];\n",
        {{"const_Integer_l1_c20", 2_i64}, {"const_Integer_l1_c69", 7_i64}}
    },
    {   // var defs are not constant.
        "var a: Integer;\nexpr b: Integer = a + 1;\n",
        "var a: Integer;\nexpr b: Integer = (a + 1);\n",
    },
};

TEST(ConstantFolding, testNonConstConst) {
    Environment env;
    test::registerBuiltIns(env);
    CompileEnv compileEnv(env, false);
    try {
        // Multiplication not registered as pure, can't be constant folded.
        Parser parser(compileEnv, "const x : Integer = 2 * 3;");
        parser.parse();
        TypeInference typeInference(compileEnv);
        typeInference.run();
        ConstantFolding constantFolding(compileEnv, false);
        constantFolding.run();
        ASSERT_TRUE(false); // LCOV_EXCL_LINE
    } catch(const CompileError& err) {
        ASSERT_STREQ("1.21-1.25: Error: Right hand side of constant 'x' is not a constant expression", err.what());
    }
}

INSTANTIATE_TEST_SUITE_P(SuiteConstantFolding,
                         TestConstFolding,
                         testing::ValuesIn(tests));

} // namespace jex
