#pragma once

#include <jex_environment.hpp>

#include <gtest/gtest.h>

// Ensure backwards compatibility for some older gtest versions.
#ifndef INSTANTIATE_TEST_SUITE_P
#define INSTANTIATE_TEST_SUITE_P(...) INSTANTIATE_TEST_CASE_P(__VA_ARGS__)
#endif

namespace jex::test {

static inline void registerBuiltIns(Environment& env) {
    jex::TypeSystem& types = env.types();
    types.registerType(jex::TypeId::Bool, "Bool", 1);
    types.registerType(jex::TypeId::Integer, "Integer", 8);
    types.registerType(jex::TypeId::Float, "Float", 8);
    types.registerType(jex::TypeId::String, "String", 2); // FIXME: how to represent strings
}

}
