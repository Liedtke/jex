cmake_minimum_required(VERSION 3.16)

project(nopressure VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -Wimplicit-fallthrough")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -Wimplicit-fallthrough")

add_subdirectory(lib)
add_subdirectory(test)
