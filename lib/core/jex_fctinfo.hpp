#pragma once

#include <jex_typeinfo.hpp>

#include <functional>
#include <iosfwd>
#include <string>
#include <type_traits>
#include <vector>

namespace jex {

class IntrinsicGen;

// Object representing an argument with variable amount of elements (at least 1).
template <typename T>
class VarArg {
    const T* args = nullptr;
    uint64_t argc = 0;

public:
    VarArg() = default;
    VarArg(const T* args, uint64_t argc)
    : args(args)
    , argc(argc) {
    }

    const T* begin() const {
        return args;
    }
    const T* end() const {
        return args + argc;
    }
    size_t size() const {
        return argc;
    }
};


enum class FctFlags {
    None = 0,
    // Function is deterministic and free of side-effects. These functions can be constant folded.
    Pure = 1 << 0,
};

inline FctFlags operator|(FctFlags lhs, FctFlags rhs) {
    using T = std::underlying_type_t<FctFlags>;
    return static_cast<FctFlags>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline FctFlags operator&(FctFlags lhs, FctFlags rhs) {
    using T = std::underlying_type_t<FctFlags>;
    return static_cast<FctFlags>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

struct ParamInfo {
    TypeInfoId type;
    bool isVarArg;

    bool operator==(const ParamInfo& other) const {
        return type == other.type && isVarArg == other.isVarArg;
    }
    bool operator!=(const ParamInfo& other) const {
        return !(*this == other);
    }
};

/**
 * Represents a function in the function library and defines its properties and behavior.
 */
class FctInfo {
public:
    using FctWrapper = void(*)(void* fctPtr, void* const* args);
    using IntrinsicFct = std::function<void(IntrinsicGen&)>;

    std::string d_name;
    std::string d_mangledName;
    std::string d_intrinsicName;
    void* d_fctPtr;
    FctWrapper d_fctWrapper;
    IntrinsicFct d_intrinsicFct;
    TypeInfoId d_retType;
    std::vector<ParamInfo> d_params;
    FctFlags d_flags;

    FctInfo(std::string name, void* fctPtr, FctWrapper fctWrapper, TypeInfoId retType, std::vector<ParamInfo> params,
            IntrinsicFct intrinsicFct = nullptr, FctFlags flags = FctFlags::None);

    bool matches(const std::vector<TypeInfoId>& argTypes) const;
    bool equals(const std::vector<ParamInfo>& params) const;

    void call(void* const* args) const {
        d_fctWrapper(d_fctPtr, args);
    }

    bool isPure() const {
        return hasFlag(FctFlags::Pure);
    }

    static void printParamTypes(std::ostream& str, const std::vector<ParamInfo>& params);
    static void printParamTypes(std::ostream& str, const std::vector<TypeInfoId>& paramTypes);

private:
    bool hasFlag(FctFlags flag) const {
        return (d_flags & flag) == flag;
    }
};

std::ostream& operator<<(std::ostream& str, const FctInfo& fctInfo);

} // namespace jex
