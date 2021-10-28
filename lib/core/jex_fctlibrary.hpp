#pragma once

#include <jex_base.hpp>
#include <jex_typeinfo.hpp>

#include <deque>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace jex {
class TypeSystem;
class FctInfo;

class FctLibrary : NoCopy {
public:
    // TODO: use string view
    using FctsByName = std::unordered_map<std::string, std::vector<FctInfo*>>;
private:
    std::deque<FctInfo> d_fctInfos;
    FctsByName d_fctsByName;
    std::unordered_map<std::string_view, FctInfo*> d_fctByMangledName;

public:
    FctLibrary() = default;
    ~FctLibrary();
    void registerFct(FctInfo&& fctInfo);
    const FctInfo& getFct(const std::string& name, const std::vector<TypeInfoId>& paramTypes) const;
    const FctInfo& getConstructor(TypeInfoId type) const;
    const FctInfo& getDestructor(TypeInfoId type) const;
    const FctInfo& getAssign(TypeInfoId type) const;


    FctsByName::const_iterator begin() const {
        return d_fctsByName.begin();
    }

    FctsByName::const_iterator end() const {
        return d_fctsByName.end();
    }
};

} // namespace jex
