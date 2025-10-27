#!/bin/bash

cd "$(dirname "$0")/.."


ls test/data
tools/compile

rm -rf test/data/tmp
mkdir -p test/data/tmp

cd test/data
../../build/lemondb --listen queries/few_insert_delete.query > lemondb.out

cat lemondb.out
echo ""
ls tmp

exit 0







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
  *) exit 0 ;;
esac
check_result "ALL" 0
exit 0
