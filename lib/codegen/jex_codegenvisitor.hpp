#pragma once

#include <jex_base.hpp>
#include <jex_basicastvisitor.hpp>
#include <jex_typeinfo.hpp>

#include "llvm/IR/IRBuilder.h"

#include <memory>
#include <unordered_map>

namespace jex {

class CodeModule;
class CompileEnv;
class FctInfo;
struct Symbol;

class CodeGenVisitor : private BasicAstVisitor, NoCopy {
    CompileEnv& d_env;
    std::unique_ptr<CodeModule> d_module;
    std::unique_ptr<llvm::IRBuilder<>> d_builder;
    llvm::Function* d_currFct = nullptr;
    std::unordered_map<const Symbol*, size_t> d_offsets;
    std::unordered_map<TypeInfoId, llvm::Type*> d_types;
    llvm::StructType* d_rctxType = nullptr;
    llvm::Value* d_result = nullptr;
public:
    CodeGenVisitor(CompileEnv& env);
    ~CodeGenVisitor();

    void createIR();
    std::unique_ptr<CodeModule> releaseModule() {
        return std::move(d_module);
    }

    void visit(AstVariableDef& node) override;
    void visit(AstLiteralExpr& node) override;
    void visit(AstBinaryExpr& node) override;
    void visit(AstFctCall& node) override;
    void visit(AstIf& node) override;

private:
    llvm::Value* visitExpression(IAstExpression& node);
    llvm::Value* getVarPtr(const Symbol* varSym);
    llvm::Type* getType(TypeInfoId type);
    llvm::StructType* createOpaqueStructType(TypeInfoId type);
    llvm::FunctionCallee getOrCreateFct(const FctInfo* fctInfo);
    void createAssign(llvm::Value* result, llvm::Value* source, TypeInfoId type);
    void createInit(const Symbol* sym);
    void createDestruct(const Symbol* sym);

    template<typename Iter>
    void createInitDestructFct(Iter symBegin, Iter symEnd, const char* prefix,
                               void(CodeGenVisitor::*createCall)(const Symbol*));
};

} // namespace jex
