#pragma once

#include <cassert>
#include <deque>
#include <set>
#include <memory>

// FIXME: Why is this required?
#include <jex_ast.hpp>

namespace jex {

struct Location;
struct MsgInfo;
class IAstNode;

class CompileEnv {
    std::set<MsgInfo> d_messages;
    std::deque<std::unique_ptr<IAstNode>> d_nodes;

public:
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
};

} // namespace jex
