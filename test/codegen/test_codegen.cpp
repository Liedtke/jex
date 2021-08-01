#include <jex_ast.hpp>
#include <jex_codemodule.hpp>
#include <jex_codegen.hpp>
#include <jex_compileenv.hpp>
#include <jex_parser.hpp>
#include <jex_typeinference.hpp>

#include "llvm/Support/raw_ostream.h"

#include <gtest/gtest.h>

namespace jex {

TEST(Codgen, dummy) {
    CompileEnv env;
    Parser parser(env, "12345");
    parser.parse();
    TypeInference typeInference(env);
    env.getRoot()->accept(typeInference);
    CodeGen codeGen(env);
    codeGen.createIR();
    // print module
    std::string result;
    llvm::raw_string_ostream irstream(result);
    irstream << *codeGen.getLlvmModule();
    ASSERT_EQ("; ModuleID = 'test'\n"
              "source_filename = \"test\"\n",
              result);
}

} // namespace jex
