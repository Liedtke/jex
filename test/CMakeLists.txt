find_package(GTest REQUIRED)

include(GoogleTest)

add_executable(unit_test
    test_lexer.cpp
    test_parser.cpp
    test_registry.cpp
    test_symboltable.cpp
    test_typeinference.cpp
    test_typesystem.cpp
)

target_include_directories(unit_test
    PRIVATE .
    PRIVATE ../include
    PRIVATE ../lib
    PRIVATE ${GTEST_INCLUDE_DIRS}
)

target_link_libraries(unit_test PRIVATE
    jex
    ${GTEST_LIBRARIES}
    gtest_main
    pthread # for gtest
)

# Instead of discovering tests, add single tests per test executable.
# This speeds up test execution significantly.
# gtest_discover_tests(unit_test)
add_test(NAME unit_test COMMAND unit_test)
