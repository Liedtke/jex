#include <jex_fctlibrary.hpp>

#include <jex_errorhandling.hpp>
#include <jex_fctinfo.hpp>

#include <sstream>

namespace jex {

FctLibrary::~FctLibrary() = default;

void FctLibrary::registerFct(FctInfo&& fctInfo) {
    std::vector<FctInfo*>& overloads = d_fctsByName[fctInfo.d_name];
    for (const FctInfo* overload : overloads) {
        if (overload->equals(fctInfo.d_params)) {
            std::stringstream err;
            err << "Duplicate function '" << fctInfo << "'";
            throw InternalError(err.str());
        }
    }
    d_fctInfos.push_back(std::move(fctInfo));
    FctInfo* ownedInfo = &d_fctInfos.back();
    overloads.push_back(ownedInfo);
    auto[iter, inserted] = d_fctByMangledName.emplace(ownedInfo->d_mangledName, ownedInfo);
    (void)iter;
    assert(inserted && "Mangled names may not clash");
}

const FctInfo& FctLibrary::getFct(const std::string& name,
                                  const std::vector<TypeInfoId>& paramTypes) const {
    const auto iter = d_fctsByName.find(name);
    if (iter == d_fctsByName.end()) {
        throw InternalError("Invalid function name '" + name + "'");
    }
    for (const FctInfo* candidate : iter->second) {
        if (candidate->matches(paramTypes)) {
            return *candidate;
        }
    }
    std::stringstream err;
    err << "No matching candidate found for function '" + name + '(';
    FctInfo::printParamTypes(err, paramTypes);
    err << ")'. Candidates are:";
    // Print candidates.
    for (const FctInfo* candidate : iter->second) {
        err << "\n  " << *candidate;
    }
    throw InternalError(err.str());
}

const FctInfo& FctLibrary::getConstructor(TypeInfoId type) const {
    return getFct("_ctor_" + type->name(), {});
}

const FctInfo& FctLibrary::getDestructor(TypeInfoId type) const {
    return getFct("_dtor_" + type->name(), {});
}

const FctInfo& FctLibrary::getAssign(TypeInfoId type) const {
    return getFct("_assign", {type});
}

} // namespace jex
