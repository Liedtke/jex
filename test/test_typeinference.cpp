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
    std::unique_ptr<CompileEnv> d_env;

public:
    void SetUp() override {
        d_env = std::make_unique<CompileEnv>();
        Registry registry(d_env->typeSystem(), d_env->fctLibrary());
        registry.registerType<ArgUInt32>();
        registry.registerFct(FctDesc<ArgUInt32, ArgUInt32>(pass, "pass"));
        registry.registerFct(FctDesc<ArgUInt32, ArgUInt32, ArgUInt32>(add, "add"));
        registry.registerFct(FctDesc<ArgUInt32, ArgUInt32, ArgUInt32>(add, "operator+"));
        registry.registerFct(FctDesc<ArgUInt32, ArgUInt32, ArgUInt32>(add, "operator-"));
        registry.registerFct(FctDesc<ArgUInt32, ArgUInt32, ArgUInt32>(add, "operator*"));
        registry.registerFct(FctDesc<ArgUInt32, ArgUInt32, ArgUInt32>(add, "operator/"));
        registry.registerFct(FctDesc<ArgUInt32, ArgUInt32, ArgUInt32>(add, "operator%"));
        // TODO: Add support for proper type inference of literals.
        d_env->symbols().addSymbol(Location(), Symbol::Kind::Variable, "x", d_env->typeSystem().getType("UInt32"));
        // TODO: Entries in the function library shouldn't have to be added explicitly to it.
        d_env->symbols().addSymbol(Location(), Symbol::Kind::Function, "pass", d_env->typeSystem().unresolved());
        d_env->symbols().addSymbol(Location(), Symbol::Kind::Function, "add", d_env->typeSystem().unresolved());
        d_env->symbols().addSymbol(Location(), Symbol::Kind::Function, "operator+", d_env->typeSystem().unresolved());
        d_env->symbols().addSymbol(Location(), Symbol::Kind::Function, "operator-", d_env->typeSystem().unresolved());
        d_env->symbols().addSymbol(Location(), Symbol::Kind::Function, "operator*", d_env->typeSystem().unresolved());
        d_env->symbols().addSymbol(Location(), Symbol::Kind::Function, "operator/", d_env->typeSystem().unresolved());
        d_env->symbols().addSymbol(Location(), Symbol::Kind::Function, "operator%", d_env->typeSystem().unresolved());
    }
};

TEST_F(TestTypeInference, resolveFct) {
    Parser parser(*d_env, "pass(x)");
    parser.parse();
    TypeInference typeInference(*d_env);
    d_env->getRoot()->accept(typeInference);
    ASSERT_FALSE(d_env->hasErrors());
}

TEST_F(TestTypeInference, resolveFctInvalidOverload) {
    Parser parser(*d_env, "pass()");
    parser.parse();
    TypeInference typeInference(*d_env);
    // TODO: Should the type inference throw in case of errors?
    d_env->getRoot()->accept(typeInference);
    ASSERT_EQ(1, d_env->messages().size());
    std::stringstream err;
    err << *d_env->messages().begin();
    ASSERT_EQ("1.1-1.6: Error: No matching candidate found for function 'pass()'", err.str());
}

TEST_F(TestTypeInference, resolveFctInvalidOverloadRepeated) {
    Parser parser(*d_env, "add(pass(pass()), add())");
    parser.parse();
    TypeInference typeInference(*d_env);
    d_env->getRoot()->accept(typeInference);
    // Only the inner errors gets reported as the outer ones are only follow-up errors.
    ASSERT_EQ(2, d_env->messages().size());
    std::stringstream err;
    auto iter = d_env->messages().begin();
    err << *iter << '\n';
    ++iter;
    err << *iter;
    ASSERT_EQ(
        "1.10-1.15: Error: No matching candidate found for function 'pass()'\n"
        "1.19-1.23: Error: No matching candidate found for function 'add()'",
        err.str());
}

TEST_F(TestTypeInference, resolveOperator) {
    Parser parser(*d_env, "x + x");
    parser.parse();
    TypeInference typeInference(*d_env);
    d_env->getRoot()->accept(typeInference);
    ASSERT_FALSE(d_env->hasErrors());
}

TEST_F(TestTypeInference, resolveOperatorAll) {
    Parser parser(*d_env, "x + x - x * x / x % x");
    parser.parse();
    TypeInference typeInference(*d_env);
    d_env->getRoot()->accept(typeInference);
    ASSERT_FALSE(d_env->hasErrors());
}

TEST_F(TestTypeInference, resolveOperatorInvalidOverload) {
    Parser parser(*d_env, "x + 1");
    parser.parse();
    TypeInference typeInference(*d_env);
    // TODO: Should the type inference throw in case of errors?
    d_env->getRoot()->accept(typeInference);
    ASSERT_EQ(1, d_env->messages().size());
    std::stringstream err;
    err << *d_env->messages().begin();
    ASSERT_EQ("1.1-1.5: Error: No matching candidate found for function 'operator+(UInt32, Integer)'", err.str());
}

TEST_F(TestTypeInference, resolveOperatorInvalidOverloadRepeated) {
    Parser parser(*d_env, "x + 1 + (1 + x)");
    parser.parse();
    TypeInference typeInference(*d_env);
    d_env->getRoot()->accept(typeInference);
    // Only the inner errors gets reported as the outer ones are only follow-up errors.
    ASSERT_EQ(2, d_env->messages().size());
    std::stringstream err;
    auto iter = d_env->messages().begin();
    err << *iter << '\n';
    ++iter;
    err << *iter;
    ASSERT_EQ(
        "1.1-1.5: Error: No matching candidate found for function 'operator+(UInt32, Integer)'\n"
        "1.10-1.14: Error: No matching candidate found for function 'operator+(Integer, UInt32)'",
        err.str());
}

} // namespace jex
