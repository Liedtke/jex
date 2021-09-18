#include <jex_codegen.hpp>

#include <jex_codegenvisitor.hpp>
#include <jex_codemodule.hpp>
#include <jex_compileenv.hpp>

#include "llvm/Passes/PassBuilder.h"

namespace jex {

static llvm::PassBuilder::OptimizationLevel toLlvmOptLevel(OptLevel level) {
    using llvmOpt = llvm::PassBuilder::OptimizationLevel;
    switch (level) {
        case OptLevel::O0: return llvmOpt::O0;
        case OptLevel::O1: return llvmOpt::O1;
        case OptLevel::O2: return llvmOpt::O2;
        case OptLevel::O3: return llvmOpt::O3;
    }
    return llvmOpt::O0; // LCOV_EXCL_LINE unreachable;
}

CodeGen::CodeGen(CompileEnv& env, OptLevel optLevel)
: d_env(env)
, d_module()
, d_optLevel(optLevel) {
}

CodeGen::~CodeGen() {
}

const llvm::Module& CodeGen::getLlvmModule() const {
    return d_module->llvmModule();
}

void CodeGen::createIR() {
    CodeGenVisitor codeGenVisitor(d_env);
    codeGenVisitor.createIR();
    // Any errors in code generation should be hard failures.
    assert(!d_env.hasErrors());
    d_module = codeGenVisitor.releaseModule();
    optimize();
}

void CodeGen::optimize() {
    if (d_optLevel == OptLevel::O0) {
        return; // Skip all optimizations.
    }
    // Build optimization pipeline.
    llvm::PassBuilder passBuilder;
    llvm::ModulePassManager passMgr = passBuilder.buildPerModuleDefaultPipeline(toLlvmOptLevel(d_optLevel));
    // Boilerplate code to have all the analysis manger objects on stack and registered.
    llvm::LoopAnalysisManager loopAnalysisManager;
    llvm::FunctionAnalysisManager functionAnalysisManager;
    llvm::CGSCCAnalysisManager cGSCCAnalysisManager;
    llvm::ModuleAnalysisManager moduleAnalysisManager;
    passBuilder.registerModuleAnalyses(moduleAnalysisManager);
    passBuilder.registerCGSCCAnalyses(cGSCCAnalysisManager);
    passBuilder.registerFunctionAnalyses(functionAnalysisManager);
    passBuilder.registerLoopAnalyses(loopAnalysisManager);
    passBuilder.crossRegisterProxies(
        loopAnalysisManager, functionAnalysisManager, cGSCCAnalysisManager, moduleAnalysisManager);
    // Run the optimization passes.
    passMgr.run(d_module->llvmModule(), moduleAnalysisManager);
}

std::unique_ptr<CodeModule> CodeGen::releaseModule() {
    return std::move(d_module);
}

} // namespace jex
