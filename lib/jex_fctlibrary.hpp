#pragma once

#include <jex_typeinfo.hpp>

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace jex {
class TypeSystem;
class FctInfo;

class FctLibrary {
    const TypeSystem& d_types;
    std::unordered_map<std::string, std::vector<FctInfo>> d_fcts;

public:
    FctLibrary(const TypeSystem& types);
    ~FctLibrary();
    void registerFct(FctInfo&& fctInfo);
    const FctInfo& getFct(std::string name, const std::vector<TypeInfoId>& paramTypes) const;
};

} // namespace jex