set(jex_sources
    jex_lexer.cpp
)

add_library(jex ${jex_sources})

target_include_directories(jex
    PUBLIC ../include
    PRIVATE .
)
