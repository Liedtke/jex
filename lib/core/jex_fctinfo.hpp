#pragma once

#include <jex_typeinfo.hpp>

#include <iosfwd>
#include <string>
#include <vector>

namespace jex {

/**
 * Represents a function in the function library and defines its properties and behavior.
 */
class FctInfo {
public:
    using FctWrapper = void(*)(void* fctPtr, void** args);

    std::string d_name;
    void* d_fctPtr;
    FctWrapper d_fctWrapper;
    TypeInfoId d_retType;
    std::vector<TypeInfoId> d_paramTypes;
public:
    FctInfo(std::string name, void* fctPtr, FctWrapper fctWrapper, TypeInfoId retType, std::vector<TypeInfoId> params);

    bool matches(const std::vector<TypeInfoId>& params) const;

    static void printParamTypes(std::ostream& str, const std::vector<TypeInfoId>& paramTypes);
};

std::ostream& operator<<(std::ostream& str, const FctInfo& fctInfo);

} // namespace jex
