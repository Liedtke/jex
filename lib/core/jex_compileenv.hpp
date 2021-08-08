#pragma once

#include <jex_base.hpp>

#include <cassert>
#include <deque>
#include <set>
#include <string>
#include <memory>

namespace jex {

struct Location;
struct MsgInfo;
class IAstNode;
class AstRoot;
class SymbolTable;
class TypeSystem;
class FctLibrary;

/**
 * Stores and provides access to any object needed during compilation.
 * The CompileEnv also handles the lifetime of the AST and other intermittent
 * artifacts required for compilation.
 */
class CompileEnv : NoCopy {
    std::string d_fileName;
    std::set<MsgInfo> d_messages;
    bool d_hasErrors = false;
    std::deque<std::unique_ptr<IAstNode>> d_nodes;
    AstRoot* d_root = nullptr;
    std::unique_ptr<TypeSystem> d_typeSystem;
    std::unique_ptr<FctLibrary> d_fctLibrary;
    std::unique_ptr<SymbolTable> d_symbolTable;
    std::deque<std::string> d_stringLiterals;
public:
    CompileEnv();
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

    const std::set<MsgInfo>& messages() const {
        return d_messages;
    }

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

    TypeSystem& typeSystem() {
        return *d_typeSystem;
    }

    FctLibrary& fctLibrary() {
        return *d_fctLibrary;
    }

    std::string_view createStringLiteral(std::string_view str);
};

} // namespace jex
