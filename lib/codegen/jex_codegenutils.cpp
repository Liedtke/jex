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

llvm::Type* CodeGenUtils::getReturnType(TypeInfoId type) {
    return getType(type)->getPointerTo();
}

llvm::FunctionCallee CodeGenUtils::getOrCreateFct(const FctInfo* fctInfo) {
    // Declare the C function.
    llvm::Type* voidTy = llvm::Type::getVoidTy(d_module.llvmContext());
    std::vector<llvm::Type*> params;
    params.push_back(getReturnType(fctInfo->d_retType));
    for (TypeInfoId paramType : fctInfo->d_paramTypes) {
        params.push_back(getParamType(paramType));
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
