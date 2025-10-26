#!/bin/bash

cd "$(dirname "$0")/.."

echo "[COMPILE]"

rm -rf build
mkdir build
cd build
cmake -DCMAKE_C_COMPILER=/usr/bin/clang-18 -DCMAKE_CXX_COMPILER=/usr/bin/clang++-18 ..
cmake --build . -- -j8

cd ..

echo "[CLANG-TIDY]"
# tools/cq-clang-tidy

echo "[CPPCHECK]"
tools/cq-cppcheck

echo "[CPPLINT]"
tools/cq-cpplint
