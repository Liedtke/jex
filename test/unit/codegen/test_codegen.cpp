#include <jex_ast.hpp>
#include <jex_builtins.hpp>
#include <jex_codemodule.hpp>
#include <jex_codegen.hpp>
#include <jex_compileenv.hpp>
#include <jex_constantfolding.hpp>
#include <jex_environment.hpp>
#include <jex_parser.hpp>
#include <jex_registry.hpp>
#include <jex_typeinference.hpp>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include <gtest/gtest.h>

namespace jex {
namespace {

/// Test struct for constant folding containing all types supported for
/// const folded value creation.
struct ConstStruct {
    bool a = true;
    int32_t b = -2;
    uint8_t c = 3;
    uint16_t d = 4;
    int64_t e = -5;
    float f = 1.23;
    double g = -4.56;
};

constexpr char ConstStructName[] = "ConstStruct";
using ArgConstStruct = ArgValue<ConstStruct, ConstStructName>;

class ConstStructModule : public Module {
    void registerTypes(Registry& registry) const override {
        registry.registerType<ArgConstStruct>(
            [](llvm::LLVMContext& ctx) {
                return llvm::StructType::create({
                    llvm::Type::getInt1Ty(ctx),
                    llvm::Type::getInt32Ty(ctx),
                    llvm::Type::getInt8Ty(ctx),
                    llvm::Type::getInt16Ty(ctx),
                    llvm::Type::getInt64Ty(ctx),
                    llvm::Type::getFloatTy(ctx),
                    llvm::Type::getDoubleTy(ctx)
                }, "ConstStruct");
            });
    }

    void registerFcts(Registry& registry) const override {
        auto createConstStruct = [](ConstStruct* res) { new(res) ConstStruct(); };
        registry.registerFct(FctDesc<ArgConstStruct>("createConstStruct", createConstStruct, NO_INTRINSIC, FctFlags::Pure));
    }
};

} // unnamed namespace

TEST(Codegen, empty) {
    Environment env;
    CompileEnv compileEnv(env);
    Parser parser(compileEnv, "");
    parser.parse();
    TypeInference typeInference(compileEnv);
    typeInference.run();
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

define void @__init_rctx(%Rctx* %rctx) {
entry:
  ret void
}

define void @__destruct_rctx(%Rctx* %rctx) {
entry:
  ret void
}
)IR";
    ASSERT_EQ(expected, result);
}

TEST(Codegen, simpleVarDef) {
    Environment env;
    env.addModule(BuiltInsModule());
    CompileEnv compileEnv(env);
    Parser parser(compileEnv,
    "expr a : Integer = 123;\n"
    "expr b : Float = 123.456;\n");
    parser.parse();
    TypeInference typeInference(compileEnv);
    typeInference.run();
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

define void @__init_rctx(%Rctx* %rctx) {
entry:
  %rctxAsBytePtr = bitcast %Rctx* %rctx to i8*
  %varPtr = getelementptr i8, i8* %rctxAsBytePtr, i64 0
  %varPtrTyped = bitcast i8* %varPtr to i64*
  store i64 0, i64* %varPtrTyped, align 4
  %rctxAsBytePtr1 = bitcast %Rctx* %rctx to i8*
  %varPtr2 = getelementptr i8, i8* %rctxAsBytePtr1, i64 8
  %varPtrTyped3 = bitcast i8* %varPtr2 to double*
  store double 0.000000e+00, double* %varPtrTyped3, align 8
  ret void
}

define void @__destruct_rctx(%Rctx* %rctx) {
entry:
  ret void
}
)IR";
    ASSERT_EQ(expected, result);
}

TEST(Codegen, operatorCall) {
    Environment env;
    env.addModule(BuiltInsModule());
    CompileEnv compileEnv(env);
    Parser parser(compileEnv,
    "expr a : Integer = 123 + 5;");
    parser.parse();
    TypeInference typeInference(compileEnv);
    typeInference.run();
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

define void @__init_rctx(%Rctx* %rctx) {
entry:
  %rctxAsBytePtr = bitcast %Rctx* %rctx to i8*
  %varPtr = getelementptr i8, i8* %rctxAsBytePtr, i64 0
  %varPtrTyped = bitcast i8* %varPtr to i64*
  store i64 0, i64* %varPtrTyped, align 4
  ret void
}

define void @__destruct_rctx(%Rctx* %rctx) {
entry:
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
    "expr a : Float = 123.2 + 5.5;");
    parser.parse();
    TypeInference typeInference(compileEnv);
    typeInference.run();
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

define void @__init_rctx(%Rctx* %rctx) {
entry:
  %rctxAsBytePtr = bitcast %Rctx* %rctx to i8*
  %varPtr = getelementptr i8, i8* %rctxAsBytePtr, i64 0
  %varPtrTyped = bitcast i8* %varPtr to double*
  store double 0.000000e+00, double* %varPtrTyped, align 8
  ret void
}

define void @__destruct_rctx(%Rctx* %rctx) {
entry:
  ret void
}
)IR";
    ASSERT_EQ(expected, result);
}

TEST(Codegen, operatorCallOptimized) {
    Environment env;
    env.addModule(BuiltInsModule());
    CompileEnv compileEnv(env);
    Parser parser(compileEnv,
    "expr a : Integer = 123 + 5 + (3 + 8);");
    parser.parse();
    TypeInference typeInference(compileEnv);
    typeInference.run();
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

; Function Attrs: nofree norecurse nounwind willreturn
define i64* @a(%Rctx* %rctx) local_unnamed_addr #0 {
entry:
  %varPtrTyped = bitcast %Rctx* %rctx to i64*
  store i64 139, i64* %varPtrTyped, align 4
  ret i64* %varPtrTyped
}

; Function Attrs: nofree norecurse nounwind willreturn writeonly
define void @__init_rctx(%Rctx* nocapture %rctx) local_unnamed_addr #1 {
entry:
  %varPtrTyped = bitcast %Rctx* %rctx to i64*
  store i64 0, i64* %varPtrTyped, align 4
  ret void
}

; Function Attrs: norecurse nounwind readnone willreturn
define void @__destruct_rctx(%Rctx* nocapture readnone %rctx) local_unnamed_addr #2 {
entry:
  ret void
}

attributes #0 = { nofree norecurse nounwind willreturn }
attributes #1 = { nofree norecurse nounwind willreturn writeonly }
attributes #2 = { norecurse nounwind readnone willreturn }
)IR";
    ASSERT_EQ(expected, result);
}

TEST(Codegen, ifExpression) {
    Environment env;
    env.addModule(BuiltInsModule());
    CompileEnv compileEnv(env, false);
    Parser parser(compileEnv,
    "expr a : Integer = if(1 < 1, 1+1, 2+2);");
    parser.parse();
    TypeInference typeInference(compileEnv);
    typeInference.run();
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

define void @__init_rctx(%Rctx* %rctx) {
entry:
  %rctxAsBytePtr = bitcast %Rctx* %rctx to i8*
  %varPtr = getelementptr i8, i8* %rctxAsBytePtr, i64 0
  %varPtrTyped = bitcast i8* %varPtr to i64*
  store i64 0, i64* %varPtrTyped, align 4
  ret void
}

define void @__destruct_rctx(%Rctx* %rctx) {
entry:
  ret void
}
)IR";
    ASSERT_EQ(expected, result);
}

TEST(Codegen, stringLiteral) {
    Environment env;
    env.addModule(BuiltInsModule());
    CompileEnv compileEnv(env);
    Parser parser(compileEnv,
    "expr a : String = \"Hello World!\";");
    parser.parse();
    TypeInference typeInference(compileEnv);
    typeInference.run();
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

@strLit_l1_c19 = external constant %String

define %String* @a(%Rctx* %rctx) {
entry:
  br label %begin

begin:                                            ; preds = %entry
  %rctxAsBytePtr = bitcast %Rctx* %rctx to i8*
  %varPtr = getelementptr i8, i8* %rctxAsBytePtr, i64 0
  %varPtrTyped = bitcast i8* %varPtr to %String*
  call void @__assign_String(%String* %varPtrTyped, %String* @strLit_l1_c19)
  ret %String* %varPtrTyped
}

declare void @__assign_String(%String*, %String*)

define void @__init_rctx(%Rctx* %rctx) {
entry:
  %rctxAsBytePtr = bitcast %Rctx* %rctx to i8*
  %varPtr = getelementptr i8, i8* %rctxAsBytePtr, i64 0
  %varPtrTyped = bitcast i8* %varPtr to %String*
  call void @__ctor_String(%String* %varPtrTyped)
  ret void
}

declare void @__ctor_String(%String*)

define void @__destruct_rctx(%Rctx* %rctx) {
entry:
  %rctxAsBytePtr = bitcast %Rctx* %rctx to i8*
  %varPtr = getelementptr i8, i8* %rctxAsBytePtr, i64 0
  %varPtrTyped = bitcast i8* %varPtr to %String*
  call void @__dtor_String(%String* %varPtrTyped)
  ret void
}

declare void @__dtor_String(%String*)
)IR";
    ASSERT_EQ(expected, result);
}

TEST(Codegen, stringExpression) {
    Environment env;
    env.addModule(BuiltInsModule());
    CompileEnv compileEnv(env);
    Parser parser(compileEnv,
    "expr a : String = substr(substr(\"Hello World!\", 6, 5), 0, 1);");
    parser.parse();
    TypeInference typeInference(compileEnv);
    typeInference.run();
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

@strLit_l1_c33 = external constant %String

define %String* @a(%Rctx* %rctx) {
entry:
  %res_substr = alloca %String, align 8
  %res_substr1 = alloca %String, align 8
  br label %begin

begin:                                            ; preds = %entry
  call void @_substr_String_Integer_Integer(%String* %res_substr, %String* @strLit_l1_c33, i64 6, i64 5)
  call void @_substr_String_Integer_Integer(%String* %res_substr1, %String* %res_substr, i64 0, i64 1)
  %rctxAsBytePtr = bitcast %Rctx* %rctx to i8*
  %varPtr = getelementptr i8, i8* %rctxAsBytePtr, i64 0
  %varPtrTyped = bitcast i8* %varPtr to %String*
  call void @__assign_String(%String* %varPtrTyped, %String* %res_substr1)
  br label %unwind

unwind:                                           ; preds = %begin
  call void @__dtor_String(%String* %res_substr1)
  call void @__dtor_String(%String* %res_substr)
  ret %String* %varPtrTyped
}

declare void @_substr_String_Integer_Integer(%String*, %String*, i64, i64)

declare void @__dtor_String(%String*)

declare void @__assign_String(%String*, %String*)

define void @__init_rctx(%Rctx* %rctx) {
entry:
  %rctxAsBytePtr = bitcast %Rctx* %rctx to i8*
  %varPtr = getelementptr i8, i8* %rctxAsBytePtr, i64 0
  %varPtrTyped = bitcast i8* %varPtr to %String*
  call void @__ctor_String(%String* %varPtrTyped)
  ret void
}

declare void @__ctor_String(%String*)

define void @__destruct_rctx(%Rctx* %rctx) {
entry:
  %rctxAsBytePtr = bitcast %Rctx* %rctx to i8*
  %varPtr = getelementptr i8, i8* %rctxAsBytePtr, i64 0
  %varPtrTyped = bitcast i8* %varPtr to %String*
  call void @__dtor_String(%String* %varPtrTyped)
  ret void
}
)IR";
    ASSERT_EQ(expected, result);
}

TEST(Codegen, unwindingIfExpression) {
    Environment env;
    env.addModule(BuiltInsModule());
    CompileEnv compileEnv(env);
    Parser parser(compileEnv,
    R"(expr a : String = if(1 < 2, substr(substr("Hello World!", 6, 5), 0, 1), "Another string");)");
    parser.parse();
    TypeInference typeInference(compileEnv);
    typeInference.run();
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

@strLit_l1_c43 = external constant %String
@strLit_l1_c73 = external constant %String

define %String* @a(%Rctx* %rctx) {
entry:
  %res_operator_lt = alloca i1, align 1
  %res_substr = alloca %String, align 8
  %res_substr2 = alloca %String, align 8
  %unw_flag = alloca i1, align 1
  br label %begin

begin:                                            ; preds = %entry
  call void @_operator_lt_Integer_Integer__intrinsic(i1* %res_operator_lt, i64 1, i64 2)
  %0 = load i1, i1* %res_operator_lt, align 1
  store i1 true, i1* %unw_flag, align 1
  br i1 %0, label %if_true, label %if_false

if_true:                                          ; preds = %begin
  call void @_substr_String_Integer_Integer(%String* %res_substr, %String* @strLit_l1_c43, i64 6, i64 5)
  call void @_substr_String_Integer_Integer(%String* %res_substr2, %String* %res_substr, i64 0, i64 1)
  br label %if_cnt

if_false:                                         ; preds = %begin
  store i1 false, i1* %unw_flag, align 1
  br label %if_cnt

if_cnt:                                           ; preds = %if_false, %if_true
  %if_res = phi %String* [ %res_substr2, %if_true ], [ @strLit_l1_c73, %if_false ]
  %rctxAsBytePtr = bitcast %Rctx* %rctx to i8*
  %varPtr = getelementptr i8, i8* %rctxAsBytePtr, i64 0
  %varPtrTyped = bitcast i8* %varPtr to %String*
  call void @__assign_String(%String* %varPtrTyped, %String* %if_res)
  br label %unwind3

unwind3:                                          ; preds = %if_cnt
  %flag_loaded = load i1, i1* %unw_flag, align 1
  br i1 %flag_loaded, label %unwind1, label %unwind

unwind1:                                          ; preds = %unwind3
  call void @__dtor_String(%String* %res_substr2)
  call void @__dtor_String(%String* %res_substr)
  br label %unwind

unwind:                                           ; preds = %unwind3, %unwind1
  ret %String* %varPtrTyped
}

define internal void @_operator_lt_Integer_Integer__intrinsic(i1* %0, i64 %1, i64 %2) {
entry:
  %result = icmp slt i64 %1, %2
  store i1 %result, i1* %0, align 1
  ret void
}

declare void @_substr_String_Integer_Integer(%String*, %String*, i64, i64)

declare void @__dtor_String(%String*)

declare void @__assign_String(%String*, %String*)

define void @__init_rctx(%Rctx* %rctx) {
entry:
  %rctxAsBytePtr = bitcast %Rctx* %rctx to i8*
  %varPtr = getelementptr i8, i8* %rctxAsBytePtr, i64 0
  %varPtrTyped = bitcast i8* %varPtr to %String*
  call void @__ctor_String(%String* %varPtrTyped)
  ret void
}

declare void @__ctor_String(%String*)

define void @__destruct_rctx(%Rctx* %rctx) {
entry:
  %rctxAsBytePtr = bitcast %Rctx* %rctx to i8*
  %varPtr = getelementptr i8, i8* %rctxAsBytePtr, i64 0
  %varPtrTyped = bitcast i8* %varPtr to %String*
  call void @__dtor_String(%String* %varPtrTyped)
  ret void
}
)IR";
    ASSERT_EQ(expected, result);
}

TEST(Codegen, constFoldedValue) {
    Environment env;
    env.addModule(BuiltInsModule());
    CompileEnv compileEnv(env);
    Parser parser(compileEnv,
    "expr a : Integer = 1 + 2 * (4 - 2);");
    parser.parse();
    TypeInference typeInference(compileEnv);
    typeInference.run();
    ConstantFolding constFolding(compileEnv, true);
    constFolding.run();
    CodeGen codeGen(compileEnv, OptLevel::O0);
    codeGen.createIR();
    // print module
    std::string result;
    llvm::raw_string_ostream irstream(result);
    irstream << *codeGen.getLlvmModule().getFunction("a");
    const char* expected =
R"IR(define i64* @a(%Rctx* %rctx) {
entry:
  br label %begin

begin:                                            ; preds = %entry
  %rctxAsBytePtr = bitcast %Rctx* %rctx to i8*
  %varPtr = getelementptr i8, i8* %rctxAsBytePtr, i64 0
  %varPtrTyped = bitcast i8* %varPtr to i64*
  store i64 5, i64* %varPtrTyped, align 4
  ret i64* %varPtrTyped
}
)IR";
    ASSERT_EQ(expected, result);
}

TEST(Codegen, constFoldedStruct) {
    Environment env;
    env.addModule(BuiltInsModule());
    env.addModule(ConstStructModule());
    CompileEnv compileEnv(env);
    Parser parser(compileEnv,
    "expr a : ConstStruct = createConstStruct();");
    parser.parse();
    TypeInference typeInference(compileEnv);
    typeInference.run();
    ConstantFolding constFolding(compileEnv, true);
    constFolding.run();
    CodeGen codeGen(compileEnv, OptLevel::O0);
    codeGen.createIR();
    // print module
    std::string result;
    llvm::raw_string_ostream irstream(result);
    irstream << *codeGen.getLlvmModule().getFunction("a");
    const char* expected =
R"IR(define %ConstStruct* @a(%Rctx* %rctx) {
entry:
  br label %begin

begin:                                            ; preds = %entry
  %rctxAsBytePtr = bitcast %Rctx* %rctx to i8*
  %varPtr = getelementptr i8, i8* %rctxAsBytePtr, i64 0
  %varPtrTyped = bitcast i8* %varPtr to %ConstStruct*
  %0 = load %ConstStruct, %ConstStruct* @const_ConstStruct_l1_c24, align 8
  store %ConstStruct %0, %ConstStruct* %varPtrTyped, align 8
  ret %ConstStruct* %varPtrTyped
}
)IR";
    ASSERT_EQ(expected, result);
    result = std::string();
    irstream << *codeGen.getLlvmModule().getNamedValue("const_ConstStruct_l1_c24");
    ASSERT_EQ("@const_ConstStruct_l1_c24 = internal constant %ConstStruct "
              "{ i1 true, i32 -2, i8 3, i16 4, i64 -5, float 0x3FF3AE1480000000, double -4.560000e+00 }",
              result);
}

} // namespace jex
