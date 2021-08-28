#include <jex_fctlibrary.hpp>

#include <jex_errorhandling.hpp>
#include <jex_fctinfo.hpp>

#include <sstream>

namespace jex {

FctLibrary::FctLibrary() {
}

FctLibrary::~FctLibrary() {
}

void FctLibrary::registerFct(FctInfo&& fctInfo) {
    std::vector<FctInfo*>& overloads = d_fctsByName[fctInfo.d_name];
    for (const FctInfo* overload : overloads) {
        // TODO: Adapt if implicit conversions support to check for equality.
        if (overload->matches(fctInfo.d_paramTypes)) {
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
    err << ")'";
    // TODO: Print candidates.
    throw InternalError(err.str());
}

} // namespace jex
