#!/usr/bin/env bash
set -euo pipefail

if [ "$(id -u)" -ne 0 ]; then
    echo "SKIP: proc runtime eBPF test requires root"
    exit 0
fi

REPO_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../.." && pwd)
PROC_BIN="$REPO_ROOT/src/sysDetector-ebpf/.output/proc/proc"
SERVER_MANIFEST="$REPO_ROOT/src/sysDetector-server/Cargo.toml"
CLI_MANIFEST="$REPO_ROOT/src/sysDetector-cli/Cargo.toml"
FIXTURE="$REPO_ROOT/src/selftest/proc/fixtures/selftest.conf"
CONFIG_DIR=/etc/sysDetector/proc
LOG_DIR=/var/log/sysDetector
LOG_FILE=$LOG_DIR/proc.log
SOCKET_FILE=/var/run/sysDetector.sock

cleanup() {
    set +e
    [ -n "${cli_pid:-}" ] && kill "$cli_pid" 2>/dev/null
    [ -n "${server_pid:-}" ] && kill "$server_pid" 2>/dev/null
    [ -n "${proc_pid:-}" ] && kill "$proc_pid" 2>/dev/null
    wait "${cli_pid:-}" "${server_pid:-}" "${proc_pid:-}" 2>/dev/null
    rm -f "$CONFIG_DIR/selftest.conf" "$SOCKET_FILE"
}
trap cleanup EXIT

mkdir -p "$CONFIG_DIR" "$LOG_DIR"
cp "$FIXTURE" "$CONFIG_DIR/selftest.conf"
config_id=$(stat -c '%i' "$CONFIG_DIR/selftest.conf")
: > "$LOG_FILE"

make -C "$REPO_ROOT/src/sysDetector-ebpf" CLANG="${CLANG:-clang-18}"
cargo build --manifest-path "$SERVER_MANIFEST"
cargo build --manifest-path "$CLI_MANIFEST"

"$PROC_BIN" &
proc_pid=$!

for _ in $(seq 1 50); do
    if grep -q "eBPF monitoring started" "$LOG_FILE" 2>/dev/null; then
        break
    fi
    sleep 0.1
done

test -S /var/run/sysDetector.sock 2>/dev/null || true
cargo run --manifest-path "$SERVER_MANIFEST" --quiet &
server_pid=$!

for _ in $(seq 1 50); do
    [ -S "$SOCKET_FILE" ] && break
    sleep 0.1
done

cargo run --manifest-path "$CLI_MANIFEST" --quiet -- proc start "$config_id"
for _ in $(seq 1 6); do
    /bin/true
    sleep 0.05
done
/bin/sh -c 'echo sysdetector suspicious argv selftest >/dev/null'
/bin/sleep 0.1

for _ in $(seq 1 50); do
    if grep -q "EXEC PID:" "$LOG_FILE" \
        && grep -q "COMM:sh" "$LOG_FILE" \
        && grep -q "EXIT PID:" "$LOG_FILE" \
        && grep -q "ARGV:" "$LOG_FILE" \
        && grep -q "ANOMALY short-lived process" "$LOG_FILE" \
        && grep -q "ANOMALY suspicious shell command" "$LOG_FILE" \
        && ! grep -q "COMM:true" "$LOG_FILE" \
        && ! grep -q "without matching exec event" "$LOG_FILE"; then
        echo "PASS: proc runtime logs configured eBPF events, argv, and anomalies"
        exit 0
    fi
    sleep 0.1
done

echo "FAIL: proc runtime did not log only configured eBPF events/argv/anomalies" >&2
tail -50 "$LOG_FILE" >&2
exit 1
