#pragma once

#include <jex_typeinfo.hpp>

#include <functional>
#include <iosfwd>
#include <string>
#include <type_traits>
#include <vector>

namespace jex {

class IntrinsicGen;

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
    std::vector<TypeInfoId> d_paramTypes;
    FctFlags d_flags;

    FctInfo(std::string name, void* fctPtr, FctWrapper fctWrapper, TypeInfoId retType, std::vector<TypeInfoId> params,
            IntrinsicFct intrinsicFct = nullptr, FctFlags flags = FctFlags::None);

    bool matches(const std::vector<TypeInfoId>& params) const;

    void call(void* const* args) const {
        d_fctWrapper(d_fctPtr, args);
    }

    bool isPure() const {
        return hasFlag(FctFlags::Pure);
    }

    static void printParamTypes(std::ostream& str, const std::vector<TypeInfoId>& paramTypes);

private:
    bool hasFlag(FctFlags flag) const {
        return (d_flags & flag) == flag;
    }
};

std::ostream& operator<<(std::ostream& str, const FctInfo& fctInfo);

} // namespace jex
