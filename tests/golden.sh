#!/bin/sh
# Golden-тесты сохраняют текущий вывод compiler для каждого исходного файла
# и сравнивают с ним последующие запуски.
#
#   tests/golden.sh update   заново записать ожидаемый вывод в tests/golden/
#   tests/golden.sh check    проверить вывод относительно tests/golden/ (режим по умолчанию)
#
# Golden-файл содержит код завершения и весь stdout compiler. После подключения
# семантического анализатора здесь сохраняются также семантические ошибки.
# Режим update используется только для намеренного изменения ожидаемого вывода.

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

# Берём все тесты tests/<group>/<name>.txt, исключая сами golden-файлы.
sources="$(find "$root/tests" -type d -name golden -prune -o -type f -name '*.txt' -print | sort)"

for source in $sources; do
    total=$((total + 1))
    relative="${source#$root/tests/}"
    golden_file="$golden/${relative%.txt}.golden"

    # Код 1 для semantic error является частью ожидаемого результата.
    exit_code=0
    actual="$("$binary" "$source" 2>&1)" || exit_code=$?

    if [ "$mode" = update ]; then
        # При обновлении сохраняем и stdout, и код ошибки semantic analyzer.
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
        # Показываем начало diff, чтобы быстро увидеть изменение ошибки или AST.
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
