add_executable(test_runtime
    test_compiler.cpp
)

target_include_directories(test_runtime
    PRIVATE .
    PRIVATE ${GTEST_INCLUDE_DIRS}
)

target_link_libraries(test_runtime PRIVATE
    jex_runtime
    ${GTEST_LIBRARIES}
    gtest_main
    pthread # for gtest
)

# Instead of discovering tests, add single tests per test executable.
# This speeds up test execution significantly.
# gtest_discover_tests(test_runtime)
add_test(NAME test_runtime COMMAND test_runtime)
