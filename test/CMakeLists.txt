find_package(GTest REQUIRED)
include(GoogleTest)

add_subdirectory(codegen)
add_subdirectory(core)
add_subdirectory(runtime)
