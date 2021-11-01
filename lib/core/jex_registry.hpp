#pragma once

#include <jex_environment.hpp>
#include <jex_fctinfo.hpp>
#include <jex_fctlibrary.hpp>
#include <jex_typesystem.hpp>

#include <algorithm>
#include <iterator>
#include <type_traits>

namespace jex {

// Variable argument (at least 1).
template <typename T>
class VarArg {
    const T* args = nullptr;
    uint64_t argc = 0;

public:
    const T* begin() const {
        return args;
    }
    const T* end() const {
        return args + argc;
    }
    size_t size() const {
        return argc;
    }
};

template<typename T, const char* _name>
struct Arg {
    static constexpr char const* name = _name;
    static constexpr TypeInfo::CallConv callConv =
        sizeof(T) > 8 ? TypeInfo::CallConv::ByPointer : TypeInfo::CallConv::ByValue;
    using Type = T;
    using RetType = T*;
    using ParamType = std::conditional_t<callConv == TypeInfo::CallConv::ByPointer, const T*, T>;
};

template <typename T, const char* _name, TypeInfo::CallConv callConv = (sizeof(T) > 8 ? TypeInfo::CallConv::ByPointer : TypeInfo::CallConv::ByValue)>
struct ArgValue : public Arg<T, _name> {
    static constexpr TypeKind kind = TypeKind::Value;
};

template <typename ArgT>
struct ArgVarArg : public ArgValue<VarArg<typename ArgT::ParamType>, ArgT::name, TypeInfo::CallConv::ByPointer> {
    using RetType = void; // VarArg returns are not supported.
    using InnerArg = ArgT;
};

template <typename T, const char* _name>
struct ArgObject : public Arg<T, _name> {
    static constexpr TypeKind kind = TypeKind::Complex;
    // Always pass by pointer as copying is non-trivial.
    static constexpr TypeInfo::CallConv callConv = TypeInfo::CallConv::ByPointer;
    using ParamType = const T*;
};

template <typename ArgT>
typename ArgT::ParamType typePtrToParamType(typename ArgT::Type* ptr) {
    if constexpr (ArgT::callConv == TypeInfo::CallConv::ByPointer) {
        return ptr;
    } else {
        return *ptr;
    }
}

template <typename ArgT>
struct GenericLifetimeFcts {
    using Type = typename ArgT::Type;
    using ParamType = typename ArgT::ParamType;
    using RetType = typename ArgT::RetType;

    static void defaultConstructor(RetType obj) {
        static_assert(std::is_default_constructible_v<Type>,
            "Default constructor required for all types used in expressions");
        new (obj) Type();
    }
};

template <typename ArgT>
struct GenericLifetimeFctsComplex : public GenericLifetimeFcts<ArgT> {
    using Type = typename ArgT::Type;
    using ParamType = typename ArgT::ParamType;
    using RetType = typename ArgT::RetType;

    static void destructor(RetType obj) noexcept {
        // Attention: The destructor is the only function where the return isn't used to create a
        // value from unitialized value but instead it expects an initialized value and "returns"
        // an unitialized one!
        obj->~Type();
    }

    static void copyConstructor(RetType target, ParamType source) {
        new (target) Type(*source);
    }

    static void moveConstructor(RetType target, ParamType source) {
        if constexpr (std::is_move_constructible_v<Type>) {
            new (target) Type(std::move(*const_cast<Type*>(source)));
        } else {
            new (target) Type(*source);
        }
    }

    static void assign(RetType target, ParamType source) {
        *target = *source;
    }

    static void moveAssign(RetType target, ParamType source) {
        if constexpr (std::is_move_assignable_v<Type>) {
            *target = std::move(*const_cast<Type*>(source));
        } else {
            *target = *source;
        }
    }
};

template <typename T>
bool isVarArgT(const ArgVarArg<T>& /*unused*/) {
    return true;
}

template <typename T>
bool isVarArgT(const T& /*unused*/) {
    return false;
}

template<typename ArgRet, typename... ArgT>
struct FctDesc {
    using FctType = void(*)(typename ArgRet::RetType, typename ArgT::ParamType...);
    using IntrinsicFct = FctInfo::IntrinsicFct;

    std::string name;
    FctType fctPtr;
    IntrinsicFct intrinsicFct;
    std::string retTypeName;
    std::vector<std::string> argTypeNames;
    std::vector<bool> isVarArg;
    FctFlags flags;

    template<typename... T, size_t... I>
    static void wrapperInner(void(*fct)(typename ArgRet::RetType, T...),
                             void* const* args,
                             std::index_sequence<I...> /*indices*/) {
        fct(reinterpret_cast<typename ArgRet::RetType>(args[0]),
            typePtrToParamType<ArgT>(reinterpret_cast<typename ArgT::Type*>(args[I+1]))...);
    }

    static void wrapper(void* fct, void* const* args) {
        // add return value "reinterpret_cast<typename ArgRet::retType>(args[0])"
        wrapperInner(reinterpret_cast<FctType>(fct), args, std::index_sequence_for<ArgT...>());
    }

    FctDesc(std::string name, FctType fctPtr, IntrinsicFct intrinsicFct = nullptr, FctFlags flags = FctFlags::None)
    : name(std::move(name))
    , fctPtr(fctPtr)
    , intrinsicFct(std::move(intrinsicFct))
    , retTypeName(ArgRet::name)
    , argTypeNames{ArgT::name...}
    , isVarArg{isVarArgT(ArgT())...}
    , flags(flags) {
    }
};

static FctInfo::IntrinsicFct NO_INTRINSIC = nullptr;

class Registry {
    TypeSystem& d_types;
    FctLibrary& d_fcts;

public:
    Registry(Environment& env)
    : d_types(env.types())
    , d_fcts(env.fctLib()) {
    }

    template <typename ArgT>
    void registerType(const TypeInfo::CreateTypeFct& fct = nullptr, bool isZeroInitialized = false) {
        d_types.registerType(ArgT::kind, ArgT::name, sizeof(typename ArgT::Type),
                             alignof(typename ArgT::Type), fct, isZeroInitialized, ArgT::callConv);
        std::string typeName(ArgT::name);
        registerFct(FctDesc<ArgT>("_ctor_" + typeName, GenericLifetimeFcts<ArgT>::defaultConstructor));
        if constexpr (ArgT::kind == TypeKind::Complex) {
            using LT = GenericLifetimeFctsComplex<ArgT>;
            registerFct(FctDesc<ArgT>("_dtor_" + typeName, LT::destructor));
            registerFct(FctDesc<ArgT, ArgT>("_assign", LT::assign));
            registerFct(FctDesc<ArgT, ArgT>("_moveAssign", LT::moveAssign));
            registerFct(FctDesc<ArgT, ArgT>("_copyCtor", LT::copyConstructor));
            registerFct(FctDesc<ArgT, ArgT>("_moveCtor", LT::moveConstructor));
        }
    }

    template <typename ...T>
    void registerFct(FctDesc<T...>&& desc) {
        // Resolve return and parameter types.
        TypeInfoId retTypeInfo = d_types.getType(desc.retTypeName);
        std::vector<ParamInfo> paramInfos;
        const auto& argTypeNames = desc.argTypeNames;
        for (size_t i = 0; i < argTypeNames.size(); ++i) {
            paramInfos.push_back({d_types.getType(argTypeNames[i]), desc.isVarArg[0]});
        }
        // Add function to function library.
        d_fcts.registerFct(FctInfo(desc.name, reinterpret_cast<void*>(desc.fctPtr), FctDesc<T...>::wrapper,
            retTypeInfo, std::move(paramInfos), desc.intrinsicFct, desc.flags));
    }
};

class Module {
public:
    virtual ~Module() = default;
    virtual void registerTypes(Registry& registry) const = 0;
    virtual void registerFcts(Registry& regitsry) const = 0;
};

} // namespace jex
