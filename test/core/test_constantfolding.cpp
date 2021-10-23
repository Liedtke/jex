#include <test_base.hpp>

#include <jex_ast.hpp>
#include <jex_compileenv.hpp>
#include <jex_constantfolding.hpp>
#include <jex_environment.hpp>
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
    ConstantFolding constantFolding(compileEnv);
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

const std::string operator"" _s(const char *in, unsigned long len) {
    return {in, len};
}

static TestConstFoldingT tests[] = {
    {   // Test folding of simple expression.
        "var x: Integer = 1 + 2 + 4 + (2 * 1) + (1 + 0);",
        "var x: Integer = (([const const_Integer_l1_c18] + (2 * 1)) + [const const_Integer_l1_c41]);\n",
        {{"const_Integer_l1_c18", 7_i64}, {"const_Integer_l1_c41", 1_i64}}
    },
    {   // Test folding of if expression true branch.
        "var x: Integer = if(true, (1+2)*3, 2+1);",
        "var x: Integer = ([const const_Integer_l1_c28] * 3);\n",
        {{"const_Integer_l1_c28", 3_i64}}
    },
    {   // Test folding of if expression false branch, continued folding.
        "var x: Integer = if(false, 2*3, 1+2) + 3;",
        "var x: Integer = [const const_Integer_l1_c18];\n",
        {{"const_Integer_l1_c18", 6_i64}}
    },
    {   // Test folding of nested if.
        "var x: Integer = if(false, 2*3, if(true, 1+2, 3+4) + 5) + 6;",
        "var x: Integer = [const const_Integer_l1_c18];\n",
        {{"const_Integer_l1_c18", 14_i64}}
    },
    {   // Test folding of if branches with non-const condition.
        "var x: Integer = if(2*2 == 3, 1+1, 2+2);",
        "var x: Integer = if(((2 * 2) == 3), [const const_Integer_l1_c31], [const const_Integer_l1_c36]);\n",
        {{"const_Integer_l1_c31", 2_i64}, {"const_Integer_l1_c36", 4_i64}}
    },
};

INSTANTIATE_TEST_SUITE_P(SuiteConstantFolding,
                         TestConstFolding,
                         testing::ValuesIn(tests));

} // namespace jex
