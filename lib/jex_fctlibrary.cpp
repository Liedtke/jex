#include <jex_fctlibrary.hpp>

#include <jex_errorhandling.hpp>
#include <jex_fctinfo.hpp>

#include <sstream>

namespace jex {

FctLibrary::FctLibrary(const TypeSystem& types)
: d_types(types)
, d_fcts() {
}

FctLibrary::~FctLibrary() {
}

void FctLibrary::registerFct(FctInfo&& fctInfo) {
    // TODO: Check for duplicates and throw.
    d_fcts[fctInfo.d_name].push_back(fctInfo);
}

const FctInfo& FctLibrary::getFct(std::string name,
                                  const std::vector<TypeInfoId>& paramTypes) const {
    const auto iter = d_fcts.find(name);
    if (iter == d_fcts.end()) {
        throw InternalError("Function '" + name + "' not registered");
    }
    for (const FctInfo& candidate : iter->second) {
        if (candidate.matches(paramTypes)) {
            return candidate;
        }
    }
    std::stringstream err;
    err << "No matching candidate found for function '" + name + '(';
    FctInfo::printParamTypes(err, paramTypes);
    err << ")'";
    throw InternalError(err.str());
}

} // namespace jex
