#pragma once

#include <jex_environment.hpp>
#include <jex_registry.hpp>

#include <gtest/gtest.h>

// Ensure backwards compatibility for some older gtest versions.
#ifndef INSTANTIATE_TEST_SUITE_P
#define INSTANTIATE_TEST_SUITE_P(...) INSTANTIATE_TEST_CASE_P(__VA_ARGS__)
#endif

namespace jex::test {

static constexpr char IntegerName[] = "Integer";
using ArgInteger = ArgValue<int64_t, IntegerName>;
static constexpr char FloatName[] = "Float";
using ArgFloat = ArgValue<double, FloatName>;
static constexpr char BoolName[] = "Bool";
using ArgBool = ArgValue<bool, BoolName>;
static constexpr char StringName[] = "String";
using ArgString = ArgObject<std::string, StringName>;

class TestModule : public Module {
    void registerTypes(Registry& registry) const override;
    void registerFcts(Registry& registry) const override;
};

inline void registerBuiltIns(Environment& env) {
    env.addModule(TestModule());
}

}
