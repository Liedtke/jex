#include <jex_symboltable.hpp>

#include <jex_ast.hpp>
#include <jex_compileenv.hpp>
#include <jex_errorhandling.hpp>

#include <gtest/gtest.h>

namespace jex {

TEST(SymbolTable, resolveUnknownSymbol) {
    CompileEnv env;
    SymbolTable& symbols = env.symbols();
    TypeInfoId unresolved = env.typeSystem().unresolved();
    AstIdentifier ident({{1, 1}, {1, 7}}, unresolved, "unknown");

    ASSERT_FALSE(symbols.resolveSymbol(&ident));
    ASSERT_EQ(1, env.messages().size());
    std::stringstream errMsg;
    errMsg << *env.messages().begin();
    ASSERT_EQ("1.1-1.7: Error: Unknown identifier 'unknown'", errMsg.str());
    ASSERT_TRUE(ident.d_symbol != nullptr);
    ASSERT_EQ(Symbol::Kind::Unresolved, ident.d_symbol->kind);
}

TEST(SymbolTable, addAndResolveSymbol) {
    CompileEnv env;
    SymbolTable& symbols = env.symbols();

    TypeInfoId typeInt = env.typeSystem().getType("Integer");
    Symbol* symbol = symbols.addSymbol({{1, 1}, {1, 4}}, Symbol::Kind::Variable, "test", typeInt);
    ASSERT_EQ(Symbol::Kind::Variable, symbol->kind);

    TypeInfoId unresolved = env.typeSystem().unresolved();
    AstIdentifier ident({{2, 1}, {2, 1}}, unresolved, "test");
    ASSERT_TRUE(symbols.resolveSymbol(&ident));
    ASSERT_EQ(symbol, ident.d_symbol);
    // Type is resolved by resolveSymbol.
    ASSERT_EQ(typeInt, ident.d_resultType);
    // There aren't any messages resported.
    ASSERT_TRUE(env.messages().empty());
}

TEST(SymbolTable, testDuplicateAdd) {
    CompileEnv env;
    SymbolTable& symbols = env.symbols();
    TypeInfoId unresolved = env.typeSystem().unresolved();
    AstIdentifier defNode({{1, 1}, {1, 4}}, unresolved, "duplicate");

    symbols.addSymbol({{1, 1}, {1, 4}}, Symbol::Kind::Variable, "duplicate", unresolved, &defNode);
    Symbol* symbol = symbols.addSymbol({{2, 1}, {2, 4}}, Symbol::Kind::Function, "duplicate", unresolved);
    ASSERT_EQ(Symbol::Kind::Unresolved, symbol->kind);
    // expect an error to be reported
    ASSERT_EQ(1, env.messages().size());
    std::stringstream errMsg;
    errMsg << *env.messages().begin();
    ASSERT_EQ(
        "2.1-2.4: Error: Duplicate identifier 'duplicate'\n"
        "1.1-1.4: Note: Previously defined here", errMsg.str());
}

} // namespace jex
