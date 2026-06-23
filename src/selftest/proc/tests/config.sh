#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../.." && pwd)
CONFIG_DIR="$REPO_ROOT/src/sysDetector-ebpf/configs/proc"
FIXTURE_DIR="$REPO_ROOT/src/selftest/proc/fixtures"
required_keys=(MONITOR_PERIOD MONITOR_SWITCH USER NAME RECOVER_COMMAND MONITOR_COMMAND STOP_COMMAND ALARM_COMMAND)

validate_config() {
    local file=$1
    local seen key

    for key in "${required_keys[@]}"; do
        if ! grep -Eq "^[[:space:]]*$key[[:space:]]*=" "$file"; then
            echo "FAIL: missing $key in $file" >&2
            return 1
        fi
    done

    while IFS='=' read -r key _; do
        key=${key%%#*}
        key=$(printf '%s' "$key" | xargs)
        [ -z "$key" ] && continue
        if printf '%s\n' "${required_keys[@]}" | grep -qx "$key"; then
            seen=$(grep -Ec "^[[:space:]]*$key[[:space:]]*=" "$file")
            if [ "$seen" -ne 1 ]; then
                echo "FAIL: duplicate $key in $file" >&2
                return 1
            fi
        else
            echo "FAIL: unknown key $key in $file" >&2
            return 1
        fi
    done < "$file"
}

for file in "$CONFIG_DIR"/*.conf "$FIXTURE_DIR"/*.conf; do
    [ -e "$file" ] || continue
    validate_config "$file"
done

echo "PASS: proc config files contain required unique keys"
