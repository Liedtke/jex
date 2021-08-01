#pragma once

#include <gtest/gtest.h>

// Ensure backwards compatibility for some older gtest versions.
#ifndef INSTANTIATE_TEST_SUITE_P
#define INSTANTIATE_TEST_SUITE_P(...) INSTANTIATE_TEST_CASE_P(__VA_ARGS__)
#endif
