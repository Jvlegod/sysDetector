#!/usr/bin/env bash
set -euo pipefail

PROC_SELFTEST_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

for test_case in "$PROC_SELFTEST_DIR"/tests/*.sh; do
    [ -e "$test_case" ] || continue
    echo "--> proc: $(basename "$test_case")"
    "$test_case"
done
