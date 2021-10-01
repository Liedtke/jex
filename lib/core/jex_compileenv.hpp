#pragma once

#include <jex_base.hpp>

#include <cassert>
#include <deque>
#include <memory>
#include <unordered_set>
#include <set>
#include <string>

namespace jex {

struct Location;
struct MsgInfo;
class IAstNode;
class AstRoot;
class SymbolTable;
class TypeSystem;
class FctLibrary;
class Environment;
class FctInfo;
class ConstantStore;

/**
 * Stores and provides access to any object needed during compilation.
 * The CompileEnv also handles the lifetime of the AST and other intermittent
 * artifacts required for compilation.
 */
class CompileEnv : NoCopy {
    std::string d_fileName;
    bool d_useIntrinsics;
    std::unique_ptr<std::set<MsgInfo>> d_messages;
    bool d_hasErrors = false;
    std::deque<std::unique_ptr<IAstNode>> d_nodes;
    AstRoot* d_root = nullptr;
    const TypeSystem& d_typeSystem;
    const FctLibrary& d_fctLibrary;
    std::unique_ptr<SymbolTable> d_symbolTable;
    std::deque<std::string> d_stringLiterals;
    std::unordered_set<const FctInfo*> d_usedFcts;
    std::unique_ptr<ConstantStore> d_constants;

    // Size of the runtime context.
    std::optional<size_t> d_contextSize;
public:
    CompileEnv(const Environment& env, bool useIntrinsics = true);
    ~CompileEnv();

    const MsgInfo& createError(const Location& loc, std::string msg);
    [[noreturn]] void throwError(const Location& loc, std::string msg);

    template <typename T, typename... Args>
    T* createNode(Args&&... args) {
        auto node = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = node.get();
        d_nodes.push_back(std::move(node));
        return ptr;
    }

    const std::string& fileName() const {
        return d_fileName;
    }

    bool useIntrinsics() const {
        return d_useIntrinsics;
    }

    const std::set<MsgInfo>& messages() const {
        return *d_messages;
    }

    std::unique_ptr<std::set<MsgInfo>> releaseMessages();

    bool hasErrors() const {
        return d_hasErrors;
    }

    void setRoot(AstRoot* root) {
        d_root = root;
    }
    AstRoot* getRoot() const {
        return d_root;
    }

    SymbolTable& symbols() {
        return *d_symbolTable;
    }

    void setContextSize(size_t size) {
        d_contextSize = size;
    }

    size_t getContextSize() const {
        assert(d_contextSize);
        return d_contextSize.value();
    }

    const TypeSystem& typeSystem() const {
        return d_typeSystem;
    }

    const FctLibrary& fctLibrary() const {
        return d_fctLibrary;
    }

    void addFctUsage(const FctInfo* fct) {
        d_usedFcts.insert(fct);
    }

    const std::unordered_set<const FctInfo*>& usedFcts() const {
        return d_usedFcts;
    }

    std::string_view createStringLiteral(std::string_view str);

    ConstantStore& constants() {
        return *d_constants;
    }

    std::unique_ptr<ConstantStore> releaseConstants();
};

} // namespace jex
