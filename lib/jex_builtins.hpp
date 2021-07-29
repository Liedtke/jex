#pragma once

#include <jex_registry.hpp>

#include <cstdint>

namespace jex {

static constexpr char IntegerName[] = "Integer";
using ArgInteger = Arg<int64_t, IntegerName, TypeId::Integer>;

static constexpr char FloatName[] = "Float";
using ArgFloat = Arg<double, FloatName, TypeId::Float>;

static constexpr char BoolName[] = "Bool";
using ArgBool = Arg<bool, BoolName, TypeId::Bool>;

// TODO: Come up with concept for strings which require some allocation / ownership /
// call convention.

class BuiltInsModule : public Module {
    void registerTypes(Registry& registry);
    void registerFcts(Registry& registry);
};

} // namespace jex
