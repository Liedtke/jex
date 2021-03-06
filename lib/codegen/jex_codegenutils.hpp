#pragma once

#include <jex_typeinfo.hpp>

#include "llvm/IR/IRBuilder.h"

namespace jex {

class CompileEnv;
class CodeModule;
class FctInfo;
struct ParamInfo;

class CodeGenUtils {
    CompileEnv& d_env;
    CodeModule& d_module;
    std::unordered_map<TypeInfoId, llvm::Type*> d_types;
public:
    CodeGenUtils(CompileEnv& env, CodeModule& module) : d_env(env), d_module(module) {
    }
    ~CodeGenUtils() = default;

    llvm::StructType* createOpaqueStructType(TypeInfoId type);
    llvm::Type* getType(TypeInfoId type);
    llvm::Type* getParamType(TypeInfoId type);
    llvm::Type* getParamType(const ParamInfo& param);
    llvm::Type* getVarArgType(TypeInfoId type);
    llvm::Type* getReturnType(TypeInfoId type);
    llvm::FunctionCallee getOrCreateFct(const FctInfo* fctInfo);
};

} // namespace jex
