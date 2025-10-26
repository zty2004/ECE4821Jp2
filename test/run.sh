#!/bin/bash

cd "$(dirname "$0")/.."
rm -rf build
mkdir build
cd build
cmake -DCMAKE_C_COMPILER=/usr/bin/clang-18 -DCMAKE_CXX_COMPILER=/usr/bin/clang++-18 ..
cmake --build . -- -j8
