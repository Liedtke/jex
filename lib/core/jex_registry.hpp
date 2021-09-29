#pragma once

#include <jex_environment.hpp>
#include <jex_fctinfo.hpp>
#include <jex_fctlibrary.hpp>
#include <jex_typesystem.hpp>

#include <algorithm>
#include <iterator>

namespace jex {
template<typename T, const char* _name>
struct Arg {
    using type = T;
    using retType = T*;
    static constexpr char const* name = _name;

    static void defaultConstructor(void* obj) {
        static_assert(std::is_default_constructible_v<T>,
            "Default constructor required for all types used in expressions");
        new (static_cast<T*>(obj)) T();
    }

    static void destructor(void* obj) noexcept {
        static_cast<T*>(obj)->~T();
    }

    // TODO: Figure out which of these operations are actually needed / useful.
    static void copyConstructor(void* target, const void* source) {
        new (static_cast<T*>(target)) T(*static_cast<const T*>(source));
    }

    static void moveConstructor(void* target, void* source) {
        if constexpr (std::is_move_constructible_v<T>) {
            new (static_cast<T*>(target)) T(std::move(*static_cast<T*>(source)));
        } else {
            copyConstructor(target, source);
        }
    }

    static void assign(void* target, const void* source) {
        *static_cast<T*>(target) = *static_cast<const T*>(source);
    }

    static void moveAssign(void* target, void* source) {
        if constexpr (std::is_move_assignable_v<T>) {
            *static_cast<T*>(target) = std::move(*static_cast<T*>(source));
        } else {
            assign(target, source);
        }
    }
};

template<typename T, const char* _name>
struct ArgValue : public Arg<T, _name> {
    static constexpr TypeKind kind = TypeKind::Value;
};

template<typename T, const char* _name>
struct ArgObject : public Arg<T, _name> {
    static constexpr TypeKind kind = TypeKind::Complex;
};

template<typename ArgRet, typename... ArgT>
struct FctDesc {
    using FctType = void(*)(typename ArgRet::retType, typename ArgT::type...);
    using IntrinsicFct = FctInfo::IntrinsicFct;

    std::string name;
    FctType fctPtr;
    IntrinsicFct intrinsicFct;
    std::string retTypeName;
    std::vector<std::string> argTypeNames;

    template<typename... T, size_t... I>
    static void wrapperInner(void(*fct)(typename ArgRet::retType, T...),
                             void** args,
                             std::index_sequence<I...>) {
        fct(reinterpret_cast<typename ArgRet::retType>(args[0]),
            *reinterpret_cast<typename ArgT::type*>(args[I+1])...);
    }

    static void wrapper(void* fct, void** args) {
        // add return value "reinterpret_cast<typename ArgRet::retType>(args[0])"
        wrapperInner(reinterpret_cast<FctType>(fct), args, std::index_sequence_for<ArgT...>());
    }

    FctDesc(std::string name, FctType fctPtr, IntrinsicFct intrinsicFct = nullptr)
    : name(std::move(name))
    , fctPtr(fctPtr)
    , intrinsicFct(std::move(intrinsicFct))
    , retTypeName(ArgRet::name)
    , argTypeNames{ArgT::name...} {
    }
};

class Registry {
    TypeSystem& d_types;
    FctLibrary& d_fcts;

public:
    Registry(Environment& env)
    : d_types(env.types())
    , d_fcts(env.fctLib()) {
    }

    template <typename ArgT>
    void registerType(TypeInfo::CreateTypeFct fct = nullptr, bool isZeroInitialized = false) {
        LifetimeFcts ltFcts = {ArgT::defaultConstructor, ArgT::destructor, ArgT::copyConstructor,
                               ArgT::moveConstructor, ArgT::assign, ArgT::moveAssign};
        d_types.registerType(ArgT::kind, ArgT::name, sizeof(typename ArgT::type),
                             alignof(typename ArgT::type), fct, ltFcts, isZeroInitialized);
    }

    template <typename ...T>
    void registerFct(FctDesc<T...>&& desc) {
        // Resolve return and parameter types.
        TypeInfoId retTypeInfo = d_types.getType(desc.retTypeName);
        std::vector<TypeInfoId> paramTypeInfos;
        std::transform(desc.argTypeNames.begin(), desc.argTypeNames.end(),
            std::back_inserter(paramTypeInfos),
            [this](const std::string& name) { return d_types.getType(name); });
        // Add function to function library.
        d_fcts.registerFct(FctInfo(desc.name, reinterpret_cast<void*>(desc.fctPtr),
                           FctDesc<T...>::wrapper, retTypeInfo, paramTypeInfos, desc.intrinsicFct));
    }
};

class Module {
public:
    virtual ~Module() = default;
    virtual void registerTypes(Registry& registry) const = 0;
    virtual void registerFcts(Registry& regitsry) const = 0;
};

} // namespace jex
