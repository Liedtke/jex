find_package(LLVM REQUIRED CONFIG)

add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(
    llvm_libs support core irreader target
)

set(codegen_sources
    jex_codegen.cpp
)

add_library(jex_codegen ${codegen_sources})

target_link_libraries(jex_codegen
    PUBLIC jex_core
    PUBLIC ${llvm_libs}
)

target_include_directories(jex_codegen
    PUBLIC .
    PRIVATE ${LLVM_INCLUDE_DIRS}
)
