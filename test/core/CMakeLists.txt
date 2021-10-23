add_executable(test_core
    test_base.cpp
    test_constantfolding.cpp
    test_lexer.cpp
    test_parser.cpp
    test_registry.cpp
    test_symboltable.cpp
    test_typeinference.cpp
    test_typesystem.cpp
)

target_include_directories(test_core
    PRIVATE .
    PRIVATE ../include
    PRIVATE ../lib/core
    PRIVATE ${GTEST_INCLUDE_DIRS}
)

target_link_libraries(test_core PRIVATE
    jex_core
    ${GTEST_LIBRARIES}
    gtest_main
    pthread # for gtest
)

# Instead of discovering tests, add single tests per test executable.
# This speeds up test execution significantly.
# gtest_discover_tests(test_core)
add_test(NAME test_core COMMAND test_core)
