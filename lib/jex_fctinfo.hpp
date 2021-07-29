#pragma once

#include <jex_typeinfo.hpp>

#include <iosfwd>
#include <string>
#include <vector>

namespace jex {

class FctInfo {
public:
    std::string d_name;
    void* d_fctPtr;
    TypeInfoId d_retType;
    std::vector<TypeInfoId> d_paramTypes;
public:
    FctInfo(std::string name, void* fctPtr, TypeInfoId retType, std::vector<TypeInfoId> params);

    bool matches(const std::vector<TypeInfoId>& params) const;

    static void printParamTypes(std::ostream& str, const std::vector<TypeInfoId>& paramTypes);
};

std::ostream& operator<<(std::ostream& str, const FctInfo& fctInfo);

} // namespace jex
