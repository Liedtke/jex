#include <jex_intrinsicgen.hpp>

namespace jex {

IntrinsicGen::IntrinsicGen(CodeModule& codeModule, llvm::Function& fct)
: d_codeModule(codeModule)
, d_fct(fct)
, d_builder(std::make_unique<llvm::IRBuilder<>>(codeModule.llvmContext())) {
    llvm::BasicBlock* block = llvm::BasicBlock::Create(d_codeModule.llvmContext(), "entry", &d_fct);
    d_builder->SetInsertPoint(block);
}

IntrinsicGen::~IntrinsicGen() {
    // Insert final "ret void;" if generator function didn't do that.
    if (std::uncaught_exceptions() == 0 && d_builder->GetInsertBlock()->getTerminator() == nullptr) {
        d_builder->CreateRetVoid();
    }
}

llvm::Value* IntrinsicGen::getStructElemPtr(llvm::Value* structPtr, int index, const llvm::Twine& name) {
    assert(structPtr->getType()->isPointerTy());
    llvm::Value* idx0 = llvm::ConstantInt::get(llvmContext(), llvm::APInt(32, 0));
    llvm::Value* idx1 = llvm::ConstantInt::get(llvmContext(), llvm::APInt(32, index));
    return d_builder->CreateGEP(structPtr, {idx0, idx1}, name);
}

} // namespace jex
