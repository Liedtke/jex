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
void pass(uint32_t* res, uint32_t in) {} // LCOV_EXCL_LINE
void add(uint32_t* res, uint32_t a, uint32_t b) {} // LCOV_EXCL_LINE

} // anonymous namespace

class TestTypeInference : public ::testing::Test {
protected:
    Environment d_env;
    std::unique_ptr<CompileEnv> d_compileEnv;

public:
    TestTypeInference() {
        Registry registry(d_env);
        registry.registerType<ArgUInt32>();
        registry.registerFct(FctDesc<ArgUInt32, ArgUInt32>(pass, "pass"));
        registry.registerFct(FctDesc<ArgUInt32, ArgUInt32, ArgUInt32>(add, "add"));
        registry.registerFct(FctDesc<ArgUInt32, ArgUInt32, ArgUInt32>(add, "operator+"));
        registry.registerFct(FctDesc<ArgUInt32, ArgUInt32, ArgUInt32>(add, "operator-"));
        registry.registerFct(FctDesc<ArgUInt32, ArgUInt32, ArgUInt32>(add, "operator*"));
        registry.registerFct(FctDesc<ArgUInt32, ArgUInt32, ArgUInt32>(add, "operator/"));
        registry.registerFct(FctDesc<ArgUInt32, ArgUInt32, ArgUInt32>(add, "operator%"));
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
    d_compileEnv->getRoot()->accept(typeInference);
    ASSERT_FALSE(d_compileEnv->hasErrors());
}

TEST_F(TestTypeInference, resolveFctInvalidOverload) {
    Parser parser(*d_compileEnv, "var a: UInt32 = pass();");
    parser.parse();
    TypeInference typeInference(*d_compileEnv);
    // TODO: Should the type inference throw in case of errors?
    d_compileEnv->getRoot()->accept(typeInference);
    ASSERT_EQ(1, d_compileEnv->messages().size());
    std::stringstream err;
    err << *d_compileEnv->messages().begin();
    ASSERT_EQ("1.17-1.22: Error: No matching candidate found for function 'pass()'", err.str());
}

TEST_F(TestTypeInference, resolveFctInvalidOverloadRepeated) {
    Parser parser(*d_compileEnv, "var a: UInt32 = add(pass(pass()), add());");
    parser.parse();
    TypeInference typeInference(*d_compileEnv);
    d_compileEnv->getRoot()->accept(typeInference);
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

TEST_F(TestTypeInference, resolveOperator) {
    Parser parser(*d_compileEnv, "var a: UInt32 = x + x;");
    parser.parse();
    TypeInference typeInference(*d_compileEnv);
    d_compileEnv->getRoot()->accept(typeInference);
    ASSERT_FALSE(d_compileEnv->hasErrors());
}

TEST_F(TestTypeInference, resolveOperatorAll) {
    Parser parser(*d_compileEnv, "var a: UInt32 = x + x - x * x / x % x;");
    parser.parse();
    TypeInference typeInference(*d_compileEnv);
    d_compileEnv->getRoot()->accept(typeInference);
    ASSERT_FALSE(d_compileEnv->hasErrors());
}

TEST_F(TestTypeInference, resolveOperatorInvalidOverload) {
    Parser parser(*d_compileEnv, "var a: UInt32 = x + 1;");
    parser.parse();
    TypeInference typeInference(*d_compileEnv);
    // TODO: Should the type inference throw in case of errors?
    d_compileEnv->getRoot()->accept(typeInference);
    ASSERT_EQ(1, d_compileEnv->messages().size());
    std::stringstream err;
    err << *d_compileEnv->messages().begin();
    ASSERT_EQ("1.17-1.21: Error: No matching candidate found for function 'operator+(UInt32, Integer)'", err.str());
}

TEST_F(TestTypeInference, resolveOperatorInvalidOverloadRepeated) {
    Parser parser(*d_compileEnv, "var a: UInt32 = x + 1 + (1 + x);");
    parser.parse();
    TypeInference typeInference(*d_compileEnv);
    d_compileEnv->getRoot()->accept(typeInference);
    // Only the inner errors gets reported as the outer ones are only follow-up errors.
    ASSERT_EQ(2, d_compileEnv->messages().size());
    std::stringstream err;
    auto iter = d_compileEnv->messages().begin();
    err << *iter << '\n';
    ++iter;
    err << *iter;
    ASSERT_EQ(
        "1.17-1.21: Error: No matching candidate found for function 'operator+(UInt32, Integer)'\n"
        "1.26-1.30: Error: No matching candidate found for function 'operator+(Integer, UInt32)'",
        err.str());
}

} // namespace jex
