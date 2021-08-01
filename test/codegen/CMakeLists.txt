add_executable(test_codegen
    test_codegen.cpp
)

target_include_directories(test_codegen
    PRIVATE .
    PRIVATE ${GTEST_INCLUDE_DIRS}
)

target_link_libraries(test_codegen PRIVATE
    jex_codegen
    ${GTEST_LIBRARIES}
    gtest_main
    pthread # for gtest
)

# Instead of discovering tests, add single tests per test executable.
# This speeds up test execution significantly.
# gtest_discover_tests(test_codegen)
add_test(NAME test_codegen COMMAND test_codegen)
