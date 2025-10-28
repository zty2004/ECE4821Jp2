#!/bin/bash

cd "$(dirname "$0")/.."
LOGFILE=$(mktemp)
trap "rm -f $LOGFILE" EXIT

check_result() {
  [ $2 -eq 0 ] && echo "[$1] PASSED" || { echo "[$1] FAILED"; return 1; }
}

# compile
run_compile() {
  echo "[COMPILE]" >> "$LOGFILE"
  tools/compile >> "$LOGFILE" 2>&1
  check_result "COMPILE" $?
}

# check file length
run_file_length() {
  echo "[FILE-LENGTH]" >> "$LOGFILE"
  tools/file-length >> "$LOGFILE" 2>&1
  check_result "FILE-LENGTH" $?
}

# cppcheck
run_cppcheck() {
  echo "[CPPCHECK]" >> "$LOGFILE"
  tools/cq-cppcheck >> "$LOGFILE" 2>&1
  check_result "CPPCHECK" $?
}

# cpplint
run_cpplint() {
  echo "[CPPLINT]" >> "$LOGFILE"
  tools/cq-cpplint >> "$LOGFILE" 2>&1
  check_result "CPPLINT" $?
}

# clang-tidy
run_clang_tidy() {
  echo "[CLANG-TIDY]" >> "$LOGFILE"
  tools/cq-clang-tidy >> "$LOGFILE" 2>&1
  check_result "CLANG-TIDY" $?
}

# run executable programme
run_exec() {
  local binary=$1
  local test=$2
  local diff_on=$3
  { time ../../build/$binary --listen queries/"${test}.query" > "${binary}-${test}.out" 2> "${binary}-${test}.err"; } 2>&1
  check_result "${binary}" $? || { tail -n 10 "${binary}-${test}.err" >&2; return 1; }
  [ -z "$diff_on" ] && return 0
  diff "${binary}-${test}.out" stdout/"${test}.out" > "${binary}-${test}.diff"
  [ ! -s "${binary}-${test}.diff" ]
  check_result "DIFF" $? || { cat "${binary}-${test}.diff" >&2; return 1; }
}

# run whole test
run_test() {
  local test=$1
  run_compile || { cat "$LOGFILE" >&2; return 1; }
  rm -rf test/data/tmp
  mkdir -p test/data/tmp
  cd test/data
  local failed=0
  run_exec lemondb-asan "${test}" || failed=1
  rm -rf tmp *.out *.err *.diff
  run_exec lemondb-msan "${test}" || failed=1
  rm -rf tmp *.out *.err *.diff
  run_exec lemondb-ubsan "${test}" || failed=1
  rm -rf tmp *.out *.err *.diff
  run_exec lemondb "${test}" 1 || failed=1
  rm -rf tmp *.out *.err *.diff
  cd ../..
  return ${failed}
}

# workflow controlled by scope
[[ $(git log -1 --pretty=format:'%s') =~ ^[a-zA-Z]+\(([a-zA-Z0-9_/-]+)\): ]] || exit 0
case "${BASH_REMATCH[1]}" in
  compile) run_compile || { cat "$LOGFILE" >&2; exit 1; } ;;
  file-length) run_file_length || { cat "$LOGFILE" >&2; exit 1; } ;;
  cppcheck) run_compile || { cat "$LOGFILE" >&2; exit 1; }; run_cppcheck || { cat "$LOGFILE" >&2; exit 1; } ;;
  cpplint) run_compile || { cat "$LOGFILE" >&2; exit 1; }; run_cpplint || { cat "$LOGFILE" >&2; exit 1; } ;;
  clang-tidy) run_compile || { cat "$LOGFILE" >&2; exit 1; }; run_clang_tidy || { cat "$LOGFILE" >&2; exit 1; } ;;
  cq)
    run_compile || { cat "$LOGFILE" >&2; exit 1; }
    failed=0
    run_file_length || failed=1
    run_cppcheck || failed=1
    run_cpplint || failed=1
    run_clang_tidy || failed=1
    [ $failed -eq 1 ] && { cat "$LOGFILE" >&2; exit 1; }
    ;;
  1|test) run_test test || exit 1 ;;
  2|few_read) run_test few_read || exit 1 ;;
  3|few_read_dup) run_test few_read_dup || exit 1 ;;
  4|few_read_update) run_test few_read_update || exit 1 ;;
  5|few_insert_delete) run_test few_insert_delete || exit 1 ;;
  6|single_read) run_test single_read || exit 1 ;;
  7|single_read_dup) run_test single_read_dup || exit 1 ;;
  8|single_read_update) run_test single_read_update || exit 1 ;;
  9|single_insert_delete) run_test single_insert_delete || exit 1 ;;
  10|many_read) run_test many_read || exit 1 ;;
  11|many_read_dup) run_test many_read_dup || exit 1 ;;
  12|many_read_update) run_test many_read_update || exit 1 ;;
  13|many_insert_delete) run_test many_insert_delete || exit 1 ;;
  *) exit 0 ;;
esac
check_result "ALL" 0
exit 0
