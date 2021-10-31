#include <jex_codegenutils.hpp>

#include <jex_codemodule.hpp>
#include <jex_compileenv.hpp>
#include <jex_fctinfo.hpp>
#include <jex_intrinsicgen.hpp>

namespace jex {

llvm::StructType* CodeGenUtils::createOpaqueStructType(TypeInfoId type) {
    assert(type->size() % type->alignment() == 0 && "Type size has to be a multiple of its alignment");
    size_t numElements = type->size() / type->alignment();
    llvm::Type* intTy = llvm::IntegerType::get(d_module.llvmContext(), type->alignment() * 8);
    return llvm::StructType::create(std::vector<llvm::Type*>(numElements, intTy), type->name());
}

llvm::Type* CodeGenUtils::getType(TypeInfoId type) {
    auto[iter, inserted] = d_types.emplace(type, nullptr);
    if (inserted) {
        const TypeInfo::CreateTypeFct& fct = type->createTypeFct();
        if (fct) {
            iter->second = fct(d_module.llvmContext());
        } else {
            assert(!llvm::StructType::getTypeByName(d_module.llvmContext(), type->name()));
            iter->second = createOpaqueStructType(type);
        }
    }
    return iter->second;
}

llvm::Type* CodeGenUtils::getParamType(TypeInfoId type) {
    llvm::Type* ty = getType(type);
    if (type->callConv() == TypeInfo::CallConv::ByValue) {
        return ty;
    } else {
        assert(type->callConv() == TypeInfo::CallConv::ByPointer);
        return ty->getPointerTo();
    }
}

llvm::Type* CodeGenUtils::getParamType(const ParamInfo& param) {
    if (param.isVarArg) {
        std::string name("_vararg_");
        name += param.type->name();
        llvm::Type* varArgTy = llvm::StructType::getTypeByName(d_module.llvmContext(), name);
        if (varArgTy) {
            return varArgTy;
        }
        // Create type.
        llvm::Type* elemPtrTy = getType(param.type)->getPointerTo();
        llvm::Type* i64Ty = llvm::Type::getInt64Ty(d_module.llvmContext());
        return llvm::StructType::create({elemPtrTy, i64Ty}, name)->getPointerTo();
    }
    return getParamType(param.type);
}

llvm::Type* CodeGenUtils::getReturnType(TypeInfoId type) {
    return getType(type)->getPointerTo();
}

llvm::FunctionCallee CodeGenUtils::getOrCreateFct(const FctInfo* fctInfo) {
    // Declare the C function.
    llvm::Type* voidTy = llvm::Type::getVoidTy(d_module.llvmContext());
    std::vector<llvm::Type*> params;
    params.push_back(getReturnType(fctInfo->d_retType));
    for (const ParamInfo& param : fctInfo->d_params) {
        params.push_back(getParamType(param));
    }
    llvm::FunctionType* fctType = llvm::FunctionType::get(voidTy, params, false);
    if (d_env.useIntrinsics() && fctInfo->d_intrinsicFct) {
        // Generate and insert intrinsic function.
        llvm::Function* fct = d_module.llvmModule().getFunction(fctInfo->d_intrinsicName);
        if (fct == nullptr) {
            // Generate intrinsic.
            fct = llvm::Function::Create(
        fctType, llvm::GlobalValue::LinkageTypes::InternalLinkage, fctInfo->d_intrinsicName, d_module.llvmModule());
            IntrinsicGen intrinsicGen(d_module, *fct);
            fctInfo->d_intrinsicFct(intrinsicGen);
        }
        return fct;
    } else {
        // Generate C function call.
        d_env.addFctUsage(fctInfo);
        return d_module.llvmModule().getOrInsertFunction(fctInfo->d_mangledName, fctType);
    }
}

} // namespace jex
