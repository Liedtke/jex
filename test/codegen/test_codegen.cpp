#include <jex_ast.hpp>
#include <jex_builtins.hpp>
#include <jex_codemodule.hpp>
#include <jex_codegen.hpp>
#include <jex_compileenv.hpp>
#include <jex_environment.hpp>
#include <jex_parser.hpp>
#include <jex_registry.hpp>
#include <jex_typeinference.hpp>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include <gtest/gtest.h>

namespace jex {

TEST(Codegen, empty) {
    Environment env;
    CompileEnv compileEnv(env);
    Parser parser(compileEnv, "");
    parser.parse();
    TypeInference typeInference(compileEnv);
    compileEnv.getRoot()->accept(typeInference);
    CodeGen codeGen(compileEnv);
    codeGen.createIR();
    // print module
    std::string result;
    llvm::raw_string_ostream irstream(result);
    irstream << codeGen.getLlvmModule();
    ASSERT_EQ("; ModuleID = 'test'\n"
              "source_filename = \"test\"\n",
              result);
}

TEST(Codegen, simpleVarDef) {
    Environment env;
    env.addModule(BuiltInsModule());
    CompileEnv compileEnv(env);
    Parser parser(compileEnv,
    "var a : Integer = 123;\n"
    "var b : Float = 123.456;\n");
    parser.parse();
    TypeInference typeInference(compileEnv);
    compileEnv.getRoot()->accept(typeInference);
    CodeGen codeGen(compileEnv);
    codeGen.createIR();
    // print module
    std::string result;
    llvm::raw_string_ostream irstream(result);
    irstream << codeGen.getLlvmModule();
    const char* expected =
R"IR(; ModuleID = 'test'
source_filename = "test"

%Rctx = type opaque

define i64* @a(%Rctx* %rctx) {
entry:
  %rctxAsBytePtr = bitcast %Rctx* %rctx to i8*
  %varPtr = getelementptr i8, i8* %rctxAsBytePtr, i64 0
  %varPtrTyped = bitcast i8* %varPtr to i64*
  store i64 123, i64* %varPtrTyped, align 4
  ret i64* %varPtrTyped
}

define double* @b(%Rctx* %rctx) {
entry:
  %rctxAsBytePtr = bitcast %Rctx* %rctx to i8*
  %varPtr = getelementptr i8, i8* %rctxAsBytePtr, i64 8
  %varPtrTyped = bitcast i8* %varPtr to double*
  store double 1.234560e+02, double* %varPtrTyped, align 8
  ret double* %varPtrTyped
}
)IR";
    ASSERT_EQ(expected, result);
}

} // namespace jex
