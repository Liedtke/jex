set(jex_sources
    jex_ast.cpp
    jex_astvisitor.cpp
    jex_errorhandling.cpp
    jex_compileenv.cpp
    jex_location.cpp
    jex_lexer.cpp
    jex_parser.cpp
    jex_prettyprinter.cpp
    jex_symboltable.cpp
    jex_typesystem.cpp
)

add_library(jex ${jex_sources})

target_include_directories(jex
    PUBLIC ../include
    PRIVATE .
)
