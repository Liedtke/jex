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

} // namespace jex
