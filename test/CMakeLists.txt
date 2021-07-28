enable_testing()
find_package(GTest REQUIRED)

include(GoogleTest)

add_executable(unit_test
    test_main.cpp
    test_lexer.cpp
    test_parser.cpp
    test_symboltable.cpp
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
    pthread # for gtest
)
