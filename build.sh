#!/usr/bin/env bash
# Сборка проекта через CMake и запуск компилятора.
#
# Использование:
#   ./build.sh                      — только собрать
#   ./build.sh tests/positive/01_variables.txt
#                                   — собрать и запустить на одном файле
#   ./build.sh --all-tests          — собрать и прогнать все тесты

set -euo pipefail

cd "$(dirname "$0")"

cmake -S . -B build
cmake --build build

COMPILER=build/compiler

if [ $# -eq 0 ]; then
    exit 0
fi

if [ "$1" = "--all-tests" ]; then
    status=0
    for f in tests/*/*.txt; do
        echo "=== $f ==="
        "$COMPILER" "$f" || status=1
    done
    exit $status
fi

exec "$COMPILER" "$@"
