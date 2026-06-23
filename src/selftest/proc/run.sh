#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)
PROC_DIR="$REPO_ROOT/src/sysDetector-ebpf"

if ! command -v clang-18 >/dev/null 2>&1 && ! command -v clang >/dev/null 2>&1; then
    echo "SKIP: clang or clang-18 is required to build proc eBPF selftest"
    exit 0
fi

clang_bin=${CLANG:-}
if [ -z "$clang_bin" ]; then
    if command -v clang-18 >/dev/null 2>&1; then
        clang_bin=clang-18
    else
        clang_bin=clang
    fi
fi

make -C "$PROC_DIR" CLANG="$clang_bin"
test -x "$PROC_DIR/proc"

echo "PASS: proc module builds"
