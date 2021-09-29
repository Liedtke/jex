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
    CodeGen codeGen(compileEnv, OptLevel::O0);
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
    CodeGen codeGen(compileEnv, OptLevel::O0);
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
  br label %begin

begin:                                            ; preds = %entry
  %rctxAsBytePtr = bitcast %Rctx* %rctx to i8*
  %varPtr = getelementptr i8, i8* %rctxAsBytePtr, i64 0
  %varPtrTyped = bitcast i8* %varPtr to i64*
  store i64 123, i64* %varPtrTyped, align 4
  ret i64* %varPtrTyped
}

define double* @b(%Rctx* %rctx) {
entry:
  br label %begin

begin:                                            ; preds = %entry
  %rctxAsBytePtr = bitcast %Rctx* %rctx to i8*
  %varPtr = getelementptr i8, i8* %rctxAsBytePtr, i64 8
  %varPtrTyped = bitcast i8* %varPtr to double*
  store double 1.234560e+02, double* %varPtrTyped, align 8
  ret double* %varPtrTyped
}
)IR";
    ASSERT_EQ(expected, result);
}

TEST(Codegen, operatorCall) {
    Environment env;
    env.addModule(BuiltInsModule());
    CompileEnv compileEnv(env);
    Parser parser(compileEnv,
    "var a : Integer = 123 + 5;");
    parser.parse();
    TypeInference typeInference(compileEnv);
    compileEnv.getRoot()->accept(typeInference);
    CodeGen codeGen(compileEnv, OptLevel::O0);
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
  %res_operator_add = alloca i64, align 8
  br label %begin

begin:                                            ; preds = %entry
  call void @_operator_add_Integer_Integer__intrinsic(i64* %res_operator_add, i64 123, i64 5)
  %0 = load i64, i64* %res_operator_add, align 4
  %rctxAsBytePtr = bitcast %Rctx* %rctx to i8*
  %varPtr = getelementptr i8, i8* %rctxAsBytePtr, i64 0
  %varPtrTyped = bitcast i8* %varPtr to i64*
  store i64 %0, i64* %varPtrTyped, align 4
  ret i64* %varPtrTyped
}

define internal void @_operator_add_Integer_Integer__intrinsic(i64* %0, i64 %1, i64 %2) {
entry:
  %result = add i64 %1, %2
  store i64 %result, i64* %0, align 4
  ret void
}
)IR";
    ASSERT_EQ(expected, result);
}

TEST(Codegen, operatorCallNoIntrinsic) {
    Environment env;
    env.addModule(BuiltInsModule());
    CompileEnv compileEnv(env, /*useIntrinsics*/false);
    Parser parser(compileEnv,
    "var a : Float = 123.2 + 5.5;");
    parser.parse();
    TypeInference typeInference(compileEnv);
    compileEnv.getRoot()->accept(typeInference);
    CodeGen codeGen(compileEnv, OptLevel::O0);
    codeGen.createIR();
    // print module
    std::string result;
    llvm::raw_string_ostream irstream(result);
    irstream << codeGen.getLlvmModule();
    const char* expected =
R"IR(; ModuleID = 'test'
source_filename = "test"

%Rctx = type opaque

define double* @a(%Rctx* %rctx) {
entry:
  %res_operator_add = alloca double, align 8
  br label %begin

begin:                                            ; preds = %entry
  call void @_operator_add_Float_Float(double* %res_operator_add, double 1.232000e+02, double 5.500000e+00)
  %0 = load double, double* %res_operator_add, align 8
  %rctxAsBytePtr = bitcast %Rctx* %rctx to i8*
  %varPtr = getelementptr i8, i8* %rctxAsBytePtr, i64 0
  %varPtrTyped = bitcast i8* %varPtr to double*
  store double %0, double* %varPtrTyped, align 8
  ret double* %varPtrTyped
}

declare void @_operator_add_Float_Float(double*, double, double)
)IR";
    ASSERT_EQ(expected, result);
}

TEST(Codegen, operatorCallOptimized) {
    Environment env;
    env.addModule(BuiltInsModule());
    CompileEnv compileEnv(env);
    Parser parser(compileEnv,
    "var a : Integer = 123 + 5 + (3 + 8);");
    parser.parse();
    TypeInference typeInference(compileEnv);
    compileEnv.getRoot()->accept(typeInference);
    CodeGen codeGen(compileEnv, OptLevel::O3);
    codeGen.createIR();
    // print module
    std::string result;
    llvm::raw_string_ostream irstream(result);
    irstream << codeGen.getLlvmModule();
    const char* expected =
R"IR(; ModuleID = 'test'
source_filename = "test"

%Rctx = type opaque

; Function Attrs: nofree norecurse nounwind
define i64* @a(%Rctx* %rctx) local_unnamed_addr #0 {
entry:
  %varPtrTyped = bitcast %Rctx* %rctx to i64*
  store i64 139, i64* %varPtrTyped, align 4
  ret i64* %varPtrTyped
}

attributes #0 = { nofree norecurse nounwind }
)IR";
    ASSERT_EQ(expected, result);
}

TEST(Codegen, ifExpression) {
    Environment env;
    env.addModule(BuiltInsModule());
    CompileEnv compileEnv(env, false);
    Parser parser(compileEnv,
    "var a : Integer = if(1 < 1, 1+1, 2+2);");
    parser.parse();
    TypeInference typeInference(compileEnv);
    compileEnv.getRoot()->accept(typeInference);
    CodeGen codeGen(compileEnv, OptLevel::O0);
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
  %res_operator_lt = alloca i1, align 1
  %res_operator_add = alloca i64, align 8
  %res_operator_add1 = alloca i64, align 8
  br label %begin

begin:                                            ; preds = %entry
  call void @_operator_lt_Integer_Integer(i1* %res_operator_lt, i64 1, i64 1)
  %0 = load i1, i1* %res_operator_lt, align 1
  br i1 %0, label %if_true, label %if_false

if_true:                                          ; preds = %begin
  call void @_operator_add_Integer_Integer(i64* %res_operator_add, i64 1, i64 1)
  %1 = load i64, i64* %res_operator_add, align 4
  br label %if_cnt

if_false:                                         ; preds = %begin
  call void @_operator_add_Integer_Integer(i64* %res_operator_add1, i64 2, i64 2)
  %2 = load i64, i64* %res_operator_add1, align 4
  br label %if_cnt

if_cnt:                                           ; preds = %if_false, %if_true
  %if_res = phi i64 [ %1, %if_true ], [ %2, %if_false ]
  %rctxAsBytePtr = bitcast %Rctx* %rctx to i8*
  %varPtr = getelementptr i8, i8* %rctxAsBytePtr, i64 0
  %varPtrTyped = bitcast i8* %varPtr to i64*
  store i64 %if_res, i64* %varPtrTyped, align 4
  ret i64* %varPtrTyped
}

declare void @_operator_lt_Integer_Integer(i1*, i64, i64)

declare void @_operator_add_Integer_Integer(i64*, i64, i64)
)IR";
    ASSERT_EQ(expected, result);
}

TEST(Codegen, stringLiteral) {
    Environment env;
    env.addModule(BuiltInsModule());
    CompileEnv compileEnv(env);
    Parser parser(compileEnv,
    "var a : String = \"Hello World!\";");
    parser.parse();
    TypeInference typeInference(compileEnv);
    compileEnv.getRoot()->accept(typeInference);
    CodeGen codeGen(compileEnv, OptLevel::O0);
    codeGen.createIR();
    // print module
    std::string result;
    llvm::raw_string_ostream irstream(result);
    irstream << codeGen.getLlvmModule();
    const char* expected =
R"IR(; ModuleID = 'test'
source_filename = "test"

%String = type { i64, i64, i64, i64 }
%Rctx = type opaque

define %String* @a(%Rctx* %rctx) {
entry:
  br label %begin

begin:                                            ; preds = %entry
  %rctxAsBytePtr = bitcast %Rctx* %rctx to i8*
  %varPtr = getelementptr i8, i8* %rctxAsBytePtr, i64 0
  %varPtrTyped = bitcast i8* %varPtr to %String*
  call void @String__assign(%String* @strLit_l1_c18, %String* %varPtrTyped)
  ret %String* %varPtrTyped
}

declare void @String__assign(%String*, %String*)
)IR";
    ASSERT_EQ(expected, result);
}

} // namespace jex
