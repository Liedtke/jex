set(core_sources
    jex_ast.cpp
    jex_astvisitor.cpp
    jex_basicastvisitor.cpp
    jex_compileenv.cpp
    jex_constantfolding.cpp
    jex_constantstore.cpp
    jex_environment.cpp
    jex_errorhandling.cpp
    jex_fctinfo.cpp
    jex_fctlibrary.cpp
    jex_location.cpp
    jex_lexer.cpp
    jex_parser.cpp
    jex_prettyprinter.cpp
    jex_registry.cpp
    jex_symboltable.cpp
    jex_typeinference.cpp
    jex_typeinfo.cpp
    jex_typesystem.cpp
)

add_library(jex_core ${core_sources})

target_include_directories(jex_core
    PUBLIC ../include
    PUBLIC .
)
