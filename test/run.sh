#!/bin/bash

cd "$(dirname "$0")/.."

git status
echo "Last commit: $(git log -1 --pretty=format:'%s')"

#echo "[COMPILE]" | tee /dev/stderr
#tools/compile

#echo "[CPPCHECK]" | tee /dev/stderr
#tools/cq-cppcheck

#echo "[CPPLINT]" | tee /dev/stderr
#tools/cq-cpplint

#echo "[CLANG-TIDY]" | tee /dev/stderr
#tools/cq-clang-tidy
