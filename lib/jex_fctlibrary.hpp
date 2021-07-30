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
    std::unordered_map<std::string, std::vector<FctInfo>> d_fcts;

public:
    FctLibrary();
    ~FctLibrary();
    void registerFct(FctInfo&& fctInfo);
    const FctInfo& getFct(std::string name, const std::vector<TypeInfoId>& paramTypes) const;
};

} // namespace jex
