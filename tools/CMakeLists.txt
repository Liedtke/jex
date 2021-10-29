add_executable(jexc jex_jexc.cpp)

target_include_directories(jexc PRIVATE .)
target_link_libraries(jexc PRIVATE jex_runtime)
