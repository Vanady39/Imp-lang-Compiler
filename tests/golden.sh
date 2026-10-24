#!/bin/sh
# Golden tests: capture the compiler's current output for every test source
# and diff later runs against it. Safety net for refactoring the parser.
#
#   tests/golden.sh update   (re)record expected output into tests/golden/
#   tests/golden.sh check    run all tests and diff against tests/golden/ (default)
#
# Each golden file holds the exit code and the full stdout of build/compiler
# (the tree printer and error messages all go to stdout). Re-run update only
# when an output change is intentional.

set -u

root="$(cd "$(dirname "$0")/.." && pwd)"
binary="$root/build/compiler"
golden="$root/tests/golden"

mode="${1:-check}"
case "$mode" in
update|check) ;;
*)
    echo "usage: tests/golden.sh [update|check]" >&2
    exit 2
    ;;
esac

if [ ! -x "$binary" ]; then
    echo "error: $binary not found; build first: cmake --build build" >&2
    exit 2
fi

tmp_actual="$(mktemp)"
tmp_expected="$(mktemp)"
trap 'rm -f "$tmp_actual" "$tmp_expected"' EXIT

total=0
failures=0

# Every tests/<group>/<name>.txt, with the golden directory itself excluded.
sources="$(find "$root/tests" -type d -name golden -prune -o -type f -name '*.txt' -print | sort)"

for source in $sources; do
    total=$((total + 1))
    relative="${source#$root/tests/}"
    golden_file="$golden/${relative%.txt}.golden"

    exit_code=0
    actual="$("$binary" "$source" 2>&1)" || exit_code=$?

    if [ "$mode" = update ]; then
        mkdir -p "$(dirname "$golden_file")"
        {
            printf '=== exit: %d ===\n' "$exit_code"
            printf '%s\n' "$actual"
        } > "$golden_file"
        echo "updated $relative"
        continue
    fi

    if [ ! -f "$golden_file" ]; then
        echo "MISSING $relative (no $golden_file; run: tests/golden.sh update)"
        failures=$((failures + 1))
        continue
    fi

    failed=0
    if [ "$(head -n 1 "$golden_file")" != "=== exit: $exit_code ===" ]; then
        failed=1
    fi
    tail -n +2 "$golden_file" > "$tmp_expected"
    printf '%s\n' "$actual" > "$tmp_actual"
    if ! diff -u "$tmp_expected" "$tmp_actual" > /dev/null 2>&1; then
        failed=1
    fi

    if [ "$failed" -ne 0 ]; then
        echo "FAIL $relative"
        diff -u "$tmp_expected" "$tmp_actual" | head -30
        failures=$((failures + 1))
    fi
done

if [ "$mode" = update ]; then
    echo "done: $total golden file(s) written to ${golden#$root/}"
    exit 0
fi

echo "checked $total test(s): $((total - failures)) passed, $failures failed"
[ "$failures" -eq 0 ]
