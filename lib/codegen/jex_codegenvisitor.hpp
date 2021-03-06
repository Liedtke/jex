#pragma once

#include <jex_base.hpp>
#include <jex_basicastvisitor.hpp>
#include <jex_typeinfo.hpp>

#include "llvm/IR/IRBuilder.h"

#include <memory>
#include <unordered_map>

namespace jex {

class CodeGenUtils;
class CodeModule;
class CompileEnv;
class FctInfo;
class Unwind;
struct Symbol;

class CodeGenVisitor : private BasicAstVisitor, NoCopy {
    CompileEnv& d_env;
    std::unique_ptr<CodeModule> d_module;
    std::unique_ptr<llvm::IRBuilder<>> d_builder;
    llvm::Function* d_currFct = nullptr;
    std::unique_ptr<CodeGenUtils> d_utils;
    std::unique_ptr<Unwind> d_unwind;
    std::unordered_map<const Symbol*, size_t> d_offsets;
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
    void visit(AstLogicalBinExpr& node) override;
    void visit(AstUnaryExpr& node) override;
    void visit(AstFctCall& node) override;
    void visit(AstIf& node) override;
    void visit(AstConstantExpr& node) override;
    void visit(AstVarArg& node) override;
    void visit(AstIdentifier& node) override;

private:
    llvm::Value* visitExpression(IAstExpression& node);
    llvm::Value* getVarPtr(const Symbol* varSym);
    void createAssign(llvm::Value* result, llvm::Value* source, TypeInfoId type);
    void createInit(const Symbol* sym);
    void createDestruct(const Symbol* sym);
    llvm::BasicBlock* createBlock(const char* name);
    llvm::Constant* createConstant(TypeInfoId typeId, const std::string& constantName);
    llvm::Constant* createConstant(llvm::Type* type, void*& valPtr, size_t& space, int level);

    void createStoreVariableFct(AstVariableDef& node);
    void createExprFct(AstVariableDef& node);

    template<typename Iter>
    void createInitDestructFct(Iter symBegin, Iter symEnd, const char* prefix,
                               void(CodeGenVisitor::*createCall)(const Symbol*));
};

} // namespace jex
