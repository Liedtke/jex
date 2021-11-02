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

using ConstantValue = std::variant<int64_t, std::string>;
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
        "var x: Integer = 1 + 2 + 4 + (2 * 1) + (1 + 0);",
        "var x: Integer = (([const_Integer_l1_c18] + (2 * 1)) + [const_Integer_l1_c41]);\n",
        {{"const_Integer_l1_c18", 7_i64}, {"const_Integer_l1_c41", 1_i64}}
    },
    {   // Test folding of if expression true branch.
        "var x: Integer = if(true, (1+2)*3, 2+1);",
        "var x: Integer = ([const_Integer_l1_c28] * 3);\n",
        {{"const_Integer_l1_c28", 3_i64}}
    },
    {   // Test folding of if expression false branch, continued folding.
        "var x: Integer = if(false, 2*3, 1+2) + 3;",
        "var x: Integer = [const_Integer_l1_c18];\n",
        {{"const_Integer_l1_c18", 6_i64}}
    },
    {   // Test folding of nested if.
        "var x: Integer = if(false, 2*3, if(true, 1+2, 3+4) + 5) + 6;",
        "var x: Integer = [const_Integer_l1_c18];\n",
        {{"const_Integer_l1_c18", 14_i64}}
    },
    {   // Test folding of nested if condition.
        "var x: Integer = if(if(false, true, false), 2*3, (1+1)*2);",
        "var x: Integer = ([const_Integer_l1_c51] * 2);\n",
        {{"const_Integer_l1_c51", 2_i64}}
    },
    {   // Test folding of if branches with non-const condition.
        "var x: Integer = if(2*2 == 3, 1+1, 2+2);",
        "var x: Integer = if(((2 * 2) == 3), [const_Integer_l1_c31], [const_Integer_l1_c36]);\n",
        {{"const_Integer_l1_c31", 2_i64}, {"const_Integer_l1_c36", 4_i64}}
    },
    {   // Test folding of complex type.
        "var x: String = substr(\"test\", 2, 2);",
        "var x: String = [const_String_l1_c17];\n",
        {{"const_String_l1_c17", "st"}}
    },
    {   // Test folding of complex type nested.
        "var x: String = substr(substr(\"This is a decently sized string for allocations\", 0, 100), 10, 6);",
        "var x: String = [const_String_l1_c17];\n",
        {{"const_String_l1_c17", "decent"}}
    },
    {   // Test folding with variable declared as const.
        "const x: String = substr(substr(\"This is a decently sized string for allocations\", 0, 100), 10, 6);",
        "const x: String = [const_String_l1_c19];\n",
        {{"const_String_l1_c19", "decent"}}
    },
    {   // Test folding with variable declared as const and const folding disabled.
        // As the varialbe is declared as const, it still has to be folded.
        "const x: String = substr(substr(\"This is a decently sized string for allocations\", 0, 100), 10, 6);",
        "const x: String = [const_String_l1_c19];\n",
        {{"const_String_l1_c19", "decent"}},
        false
    },
    {   // Test folding with const folding disabled.
        // As the varialbe is not declared as const, it may not be folded.
        "var x: Integer = 1 + 1;",
        "var x: Integer = (1 + 1);\n",
        {},
        false
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
