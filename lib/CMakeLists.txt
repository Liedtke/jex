set(jex_sources
    jex_astvisitor.cpp
    jex_errorhandling.cpp
    jex_compileenv.cpp
    jex_location.cpp
    jex_lexer.cpp
    jex_parser.cpp
)

add_library(jex ${jex_sources})

target_include_directories(jex
    PUBLIC ../include
    PRIVATE .
)
