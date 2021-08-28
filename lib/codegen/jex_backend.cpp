#include <jex_backend.hpp>

#include <jex_codemodule.hpp>
#include <jex_compileenv.hpp>
#include <jex_errorhandling.hpp>

#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetSelect.h"

namespace jex {

[[noreturn]] static void throwLlvmError(llvm::Error err, std::string_view errPrefix) {
    std::string errMsg;
    llvm::raw_string_ostream errStr(errMsg);
    errStr << errPrefix << ": ";
    errStr << err;
    throw InternalError(errMsg);
}

static void checked(llvm::Error err, std::string_view errPrefix) {
    if (err) {
        throwLlvmError(std::move(err), errPrefix);
    }
}

template <typename T>
static T checked(llvm::Expected<T> expectedObj, std::string_view errPrefix) {
    if (!expectedObj) {
        throwLlvmError(std::move(expectedObj.takeError()), errPrefix);
    }
    return std::move(*expectedObj);
}

Backend::Backend(CompileEnv& env)
: d_env(env) {
}

Backend::~Backend() {
}

void Backend::jit(std::unique_ptr<CodeModule> module) {
    module->llvmModule().setTargetTriple(llvm::sys::getDefaultTargetTriple());
    d_jit = checked(llvm::orc::LLJITBuilder().create(), "Error creating LLJITBuilder");
    checked(d_jit->addIRModule(llvm::orc::ThreadSafeModule(module->releaseModule(), module->releaseContext())),
            "Error adding IR module");
}

uintptr_t Backend::getFctPtr(std::string_view fctName) {
    llvm::JITEvaluatedSymbol sym = checked(d_jit->lookup(fctName), "Error looking up symbol: ");
    return static_cast<uintptr_t>(sym.getAddress());
}

void Backend::initialize() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
}

} // namespace jex
