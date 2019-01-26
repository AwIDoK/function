cmake_minimum_required(VERSION 3.13)
project(function)

set(CMAKE_CXX_STANDARD 17)

add_library(function function.cpp function.h)


add_executable(tests function.cpp function.h main.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined -D__GLIBCXX_DEBUG -Wall -Wextra")