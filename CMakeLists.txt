project(lemondb)
cmake_minimum_required(VERSION 2.7)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-std=c++20 -Wall -Wextra -Werror -pedantic -Wno-unused-result -Wconversion -Wno-unused-parameter -O2")
set(CMAKE_CXX_CLANG_TIDY "clang-tidy-18;-checks=-*,clang-analyzer-*,performance-*,misc-*,-performance-avoid-endl,-misc-include-cleaner;-fix")
set(CMAKE_CXX_CPPCHECK "cppcheck;--quiet")

file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "src/*.h")
add_executable(lemondb src/main.cpp ${SOURCE_FILES})
target_link_libraries(lemondb pthread)
