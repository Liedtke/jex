set(runtime_sources
    jex_compiler.cpp
)

add_library(jex_runtime ${runtime_sources})

target_include_directories(jex_runtime
    PUBLIC .
)

target_link_libraries(jex_runtime
    PUBLIC jex_core
    PUBLIC jex_codegen
)
