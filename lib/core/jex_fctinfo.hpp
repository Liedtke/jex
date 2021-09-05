#pragma once

#include <jex_typeinfo.hpp>

#include <functional>
#include <iosfwd>
#include <string>
#include <vector>

namespace jex {

class IntrinsicGen;

/**
 * Represents a function in the function library and defines its properties and behavior.
 */
class FctInfo {
public:
    using FctWrapper = void(*)(void* fctPtr, void** args);
    using IntrinsicFct = std::function<void(IntrinsicGen&)>;

    std::string d_name;
    std::string d_mangledName;
    std::string d_intrinsicName;
    void* d_fctPtr;
    FctWrapper d_fctWrapper;
    IntrinsicFct d_intrinsicFct;
    TypeInfoId d_retType;
    std::vector<TypeInfoId> d_paramTypes;
public:
    FctInfo(std::string name, void* fctPtr, FctWrapper fctWrapper, TypeInfoId retType, std::vector<TypeInfoId> params, IntrinsicFct intrinsicFct = nullptr);

    bool matches(const std::vector<TypeInfoId>& params) const;

    static void printParamTypes(std::ostream& str, const std::vector<TypeInfoId>& paramTypes);
};

std::ostream& operator<<(std::ostream& str, const FctInfo& fctInfo);

} // namespace jex
