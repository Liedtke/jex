#pragma once

#include <jex_base.hpp>
#include <jex_typeinfo.hpp>

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace jex {
class TypeSystem;
class FctInfo;

class FctLibrary : NoCopy {
public:
    using FctsByName = std::unordered_map<std::string, std::vector<FctInfo>>;
private:
    FctsByName d_fcts;

public:
    FctLibrary();
    ~FctLibrary();
    void registerFct(FctInfo&& fctInfo);
    const FctInfo& getFct(const std::string& name, const std::vector<TypeInfoId>& paramTypes) const;

    FctsByName::const_iterator begin() const {
        return d_fcts.begin();
    }

    FctsByName::const_iterator end() const {
        return d_fcts.end();
    }
};

} // namespace jex
