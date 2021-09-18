#pragma once

#include <jex_base.hpp>

#include <cassert>
#include <iosfwd>
#include <memory>
#include <set>

namespace llvm::orc {
    class LLJIT;
    class ThreadSafeModule;
}

namespace jex {

class CodeModule;
class CompileEnv;
struct MsgInfo;

class CompileResult {
    friend class Backend;
    friend class Compiler;

    std::unique_ptr<std::set<MsgInfo>> d_messages;
    std::unique_ptr<llvm::orc::LLJIT> d_jit;
    size_t d_contextSize = 0;

    CompileResult(std::unique_ptr<std::set<MsgInfo>> messages,
                  std::unique_ptr<llvm::orc::LLJIT> jit,
                  size_t                            contextSize);
    CompileResult(std::unique_ptr<std::set<MsgInfo>> messages);

public:
    CompileResult();
    CompileResult(CompileResult&& other);
    CompileResult(const CompileResult& other) = delete;
    CompileResult& operator=(const CompileResult& other) = delete;
    ~CompileResult();

    explicit operator bool() const {
        return static_cast<bool>(d_jit);
    }

    const std::set<MsgInfo>& getMessages() const {
        return *d_messages;
    }

    size_t getContextSize() const {
        assert(*this); // May not be called if the compile result isn't valid.
        return d_contextSize;
    }

    uintptr_t getFctPtr(std::string_view fctName);
};

std::ostream& operator<<(std::ostream& str, const CompileResult& compileResult);

class Backend :  NoCopy {
    CompileEnv& d_env;

public:
    static void initialize();

    Backend(CompileEnv& env);
    ~Backend();

    CompileResult jit(std::unique_ptr<CodeModule> module);
};

} // namespace jex
