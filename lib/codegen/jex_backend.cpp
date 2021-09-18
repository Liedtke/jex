#include <jex_backend.hpp>

#include <jex_codemodule.hpp>
#include <jex_compileenv.hpp>
#include <jex_errorhandling.hpp>
#include <jex_fctinfo.hpp>
#include <jex_fctlibrary.hpp>

#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetSelect.h"

namespace jex {

[[noreturn]] static void throwLlvmError(llvm::Error err, const char* errPrefix) {
    std::string errMsg;
    llvm::raw_string_ostream errStr(errMsg);
    llvm::logAllUnhandledErrors(std::move(err), errStr, errPrefix);
    throw InternalError(errMsg);
}

static void checked(llvm::Error err, const char* errPrefix) {
    if (err) {
        throwLlvmError(std::move(err), errPrefix);
    }
}

template <typename T>
static T checked(llvm::Expected<T> expectedObj, const char* errPrefix) {
    if (!expectedObj) {
        throwLlvmError(expectedObj.takeError(), errPrefix);
    }
    return std::move(*expectedObj);
}


CompileResult::CompileResult() = default;
CompileResult::CompileResult(CompileResult&& other) = default;
CompileResult::~CompileResult() = default;

CompileResult::CompileResult(std::unique_ptr<std::set<MsgInfo>> messages,
                             std::unique_ptr<llvm::orc::LLJIT> jit)
: d_messages(std::move(messages))
, d_jit(std::move(jit)) {
}

CompileResult::CompileResult(std::unique_ptr<std::set<MsgInfo>> messages)
: d_messages(std::move(messages)) {}

uintptr_t CompileResult::getFctPtr(std::string_view fctName) {
    if (!d_jit) {
        throw InternalError("Cannot get function pointer as compilation failed.");
    }
    llvm::JITEvaluatedSymbol sym = checked(d_jit->lookup(fctName), "Error looking up function pointer: ");
    return static_cast<uintptr_t>(sym.getAddress());
}

std::ostream& operator<<(std::ostream& str, const CompileResult& compileResult) {
    for (const MsgInfo& msg : compileResult.getMessages()) {
        str << msg;
    }
    return str;
}

Backend::Backend(CompileEnv& env)
: d_env(env) {
    initialize();
}

Backend::~Backend() {
}

CompileResult Backend::jit(std::unique_ptr<CodeModule> module) {
    // Create lljit and add IR module.
    module->llvmModule().setTargetTriple(llvm::sys::getDefaultTargetTriple());
    std::unique_ptr<llvm::orc::LLJIT> jit = checked(llvm::orc::LLJITBuilder().create(), "Error creating LLJITBuilder: ");
    checked(jit->addIRModule(llvm::orc::ThreadSafeModule(module->releaseModule(), module->releaseContext())),
            "Error adding IR module: ");
    // Create library for functions registered in the function library.
    // Add all functions used in the module.
    llvm::orc::JITDylib& lib = jit->getMainJITDylib();
    llvm::orc::ExecutionSession& es = jit->getExecutionSession();
    llvm::orc::SymbolMap symbols;
    for (const FctInfo* fct : d_env.usedFcts()) {
        symbols.insert(std::make_pair(es.intern(fct->d_mangledName), llvm::JITEvaluatedSymbol::fromPointer(fct->d_fctPtr)));
    }
    checked(lib.define(absoluteSymbols(symbols)), "Error adding fct symbols: ");
    return CompileResult(d_env.releaseMessages(), std::move(jit));
}

void Backend::initialize() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
}

} // namespace jex
