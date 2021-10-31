#include <jex_fctinfo.hpp>

#include <iostream>

namespace jex {

static std::string createMangledName(const FctInfo* fctInfo) {
    std::string name("_");
    name += fctInfo->d_name;
    for (const ParamInfo& param : fctInfo->d_params) {
        name += (param.isVarArg ? "_vararg_" : "_") + param.type->name();
    }
    return name;
}

FctInfo::FctInfo(std::string name, void* fctPtr, FctWrapper fctWrapper, TypeInfoId retType,
                 std::vector<ParamInfo> params, IntrinsicFct intrinsicFct, FctFlags flags)
: d_name(std::move(name))
, d_mangledName()
, d_intrinsicName()
, d_fctPtr(fctPtr)
, d_fctWrapper(fctWrapper)
, d_intrinsicFct(std::move(intrinsicFct))
, d_retType(retType)
, d_params(std::move(params))
, d_flags(flags) {
    d_mangledName = createMangledName(this);
    d_intrinsicName = d_mangledName + "__intrinsic";
}

bool FctInfo::matches(const std::vector<TypeInfoId>& argTypes) const {
    auto argIter = argTypes.begin();
    for (const ParamInfo& param : d_params) {
        if (argIter == argTypes.end()) {
            return false;
        }
        if (*argIter != param.type) {
            return false;
        }
        ++argIter;
        // Implement VarArg as greedy for now.
        while (param.isVarArg && *argIter == param.type) {
            ++argIter;
        }
    }
    return argIter == argTypes.end();
}

bool FctInfo::equals(const std::vector<ParamInfo>& params) const {
    if (params.size() != d_params.size()) {
        return false;
    }
    for (size_t i = 0; i < d_params.size(); ++i) {
        if (params[i] != d_params[i]) {
            return false;
        }
    }
    // All parameters are equal.
    return true;
}

void FctInfo::printParamTypes(std::ostream& str, const std::vector<ParamInfo>& params) {
    for (const ParamInfo& param: params) {
        if (&param != params.data()) {
            str << ", ";
        }
        if (param.isVarArg) {
            str << "_VarArg<" << param.type->name() << ">";
        } else {
            str << param.type->name();
        }
    }
}

void FctInfo::printParamTypes(std::ostream& str, const std::vector<TypeInfoId>& paramTypes) {
    for (const TypeInfoId& type: paramTypes) {
        if (&type != paramTypes.data()) {
            str << ", ";
        }
        str << type->name();
    }
}

std::ostream& operator<<(std::ostream& str, const FctInfo& fctInfo) {
    return str << fctInfo.d_retType->name() << ' ' << fctInfo.d_name << '(';
    FctInfo::printParamTypes(str, fctInfo.d_params);
    return str << ')';
}

} // namespace jex
