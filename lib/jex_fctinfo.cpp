#include <jex_fctinfo.hpp>

#include <iostream>

namespace jex {

FctInfo::FctInfo(std::string name, void* fctPtr, TypeInfoId retType, std::vector<TypeInfoId> params)
: d_name(std::move(name))
, d_fctPtr(fctPtr)
, d_retType(retType)
, d_paramTypes(std::move(params)) {
}

bool FctInfo::matches(const std::vector<TypeInfoId>& params) const {
    if (params.size() != d_paramTypes.size()) {
        return false;
    }
    for (size_t i = 0; i < d_paramTypes.size(); ++i) {
        // Current check: Signatures only match if exactly equivalent.
        // This means that no implicit conversions are supported.
        if (params[i] != d_paramTypes[i]) {
            return false;
        }
    }
    // All parameters match.
    return true;
}

void FctInfo::printParamTypes(std::ostream& str, const std::vector<TypeInfoId>& paramTypes) {
    for (const TypeInfoId& ti : paramTypes) {
        if (&ti != paramTypes.data()) {
            str << ", ";
        }
        str << ti->name();
    }
}

std::ostream& operator<<(std::ostream& str, const FctInfo& fctInfo) {
    return str << fctInfo.d_retType->name() << ' ' << fctInfo.d_name << '(';
    FctInfo::printParamTypes(str, fctInfo.d_paramTypes);
    return str << ')';
}

} // namespace jex
