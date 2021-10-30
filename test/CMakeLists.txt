find_package(GTest REQUIRED)
include(GoogleTest)

add_subdirectory(unit/codegen)
add_subdirectory(unit/core)
add_subdirectory(unit/runtime)
add_subdirectory(integration)
