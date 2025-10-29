cmake_minimum_required(VERSION 3.16)
project(lemondb)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER /usr/lib/llvm-18/bin/clang++)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "-std=c++20 -Wall -Wextra -Werror -pedantic -Wno-unused-result -Wconversion -Wno-unused-parameter")
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

# lemondb
add_executable(lemondb ${SOURCES})
target_link_libraries(lemondb pthread)
target_compile_options(lemondb PRIVATE -O2)

# lemondb-asan
add_executable(lemondb-asan ${SOURCES})
target_link_libraries(lemondb-asan pthread)
target_compile_options(lemondb-asan PRIVATE -DDEBUG -g -O2 -fsanitize=address -fno-omit-frame-pointer -fno-sanitize-recover=all)
target_link_options(lemondb-asan PRIVATE -fsanitize=address)

# lemondb-msan
add_executable(lemondb-msan ${SOURCES})
target_link_libraries(lemondb-msan pthread c++ c++abi)
target_compile_options(lemondb-msan PRIVATE -DDEBUG -g -O2 -fsanitize=memory -fno-omit-frame-pointer -fsanitize-memory-track-origins -fPIE -nostdinc++ -isystem /usr/local/lib/libc++_msan-18/include/c++/v1)
target_link_options(lemondb-msan PRIVATE -fsanitize=memory -pie -Wl,-rpath,/usr/local/lib/libc++_msan-18/lib)
target_link_directories(lemondb-msan PRIVATE /usr/local/lib/libc++_msan-18/lib)

# lemondb-ubsan
add_executable(lemondb-ubsan ${SOURCES})
target_link_libraries(lemondb-ubsan pthread)
target_compile_options(lemondb-ubsan PRIVATE -DDEBUG -g -O2 -fsanitize=undefined,integer,bounds,float-divide-by-zero,implicit-conversion,float-cast-overflow,nullability -fno-omit-frame-pointer -fno-sanitize-recover=all)
target_link_options(lemondb-ubsan PRIVATE -fsanitize=undefined)
