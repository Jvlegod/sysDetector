#!/usr/bin/env bash
set -euo pipefail

SELFTEST_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

modules=("$@")
if [ "${#modules[@]}" -eq 0 ]; then
    modules=(proc fs)
fi

for module in "${modules[@]}"; do
    runner="$SELFTEST_DIR/$module/run.sh"
    if [ ! -x "$runner" ]; then
        echo "SKIP: $module has no executable run.sh"
        continue
    fi

    echo "==> selftest: $module"
    "$runner"
done
