#pragma once

#include <cassert>
#include <deque>
#include <set>
#include <memory>

namespace jex {

struct Location;
struct MsgInfo;
class IAstNode;
class IAstExpression;
class SymbolTable;
class TypeSystem;

class CompileEnv {
    std::set<MsgInfo> d_messages;
    std::deque<std::unique_ptr<IAstNode>> d_nodes;
    IAstExpression* d_root = nullptr;
    std::unique_ptr<TypeSystem> d_typeSystem;
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

    const std::set<MsgInfo>& messages() const {
        return d_messages;
    }

    void setRoot(IAstExpression* root) {
        d_root = root;
    }
    IAstExpression* getRoot() const {
        return d_root;
    }

    SymbolTable& symbols() {
        return *d_symbolTable;
    }

    TypeSystem& typeSystem() {
        return *d_typeSystem;
    }

    std::string_view createStringLiteral(std::string_view str);
};

} // namespace jex
