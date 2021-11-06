find_package(LLVM 12 REQUIRED CONFIG)

add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(
    llvm_libs core irreader
    target native nativecodegen
    orcjit support
)

set(codegen_sources
    jex_backend.cpp
    jex_builtins.cpp
    jex_codegen.cpp
    jex_codegenutils.cpp
    jex_codegenvisitor.cpp
    jex_codemodule.cpp
    jex_executioncontext.cpp
    jex_intrinsicgen.cpp
    jex_unwind.cpp
)

add_library(jex_codegen ${codegen_sources})

target_link_libraries(jex_codegen
    PUBLIC jex_core
    PUBLIC ${llvm_libs}
)

target_include_directories(jex_codegen
    PUBLIC .
    PUBLIC ${LLVM_INCLUDE_DIRS}
)
