# FS monitoring configuration

The first FS monitor implementation watches configured directories with inotify and records write/delete/rename/attribute-change events.

## Fields

| Field | Description | Example |
| --- | --- | --- |
| `MONITOR_SWITCH` | Whether this config is enabled. Use `fs start <id>` / `fs stop <id>` to change it. | `off` |
| `NAME` | Human-readable rule name. | `tmp-write` |
| `PATH` | Directory path to watch. | `/tmp/sysdetector-fs-test` |
| `OPERATIONS` | Documented operations for this config. The first implementation records write/delete/rename/chmod-like events. | `write,delete,rename,chmod` |
| `ALARM_COMMAND` | Command executed after a matching event. Use `true` to disable external action. | `true` |

## Manual test

```bash
sudo mkdir -p /etc/sysDetector/fs /tmp/sysdetector-fs-test
sudo tee /etc/sysDetector/fs/tmp.conf >/dev/null <<'EOF'
MONITOR_SWITCH=off
NAME=tmp-write
PATH=/tmp/sysdetector-fs-test
OPERATIONS=write,delete,rename,chmod
ALARM_COMMAND=true
EOF

sudo systemctl restart sysDetector-fs.service
fs_id=$(stat -c '%i' /etc/sysDetector/fs/tmp.conf)
sudo sysDetector-cli fs start "$fs_id"

echo hello >/tmp/sysdetector-fs-test/a.txt
mv /tmp/sysdetector-fs-test/a.txt /tmp/sysdetector-fs-test/b.txt
chmod 600 /tmp/sysdetector-fs-test/b.txt
rm /tmp/sysdetector-fs-test/b.txt

sudo sysDetector-cli fs list events
sudo tail -100 /var/log/sysDetector/fs.log
```

Expected output includes `FS EVENT` lines for the configured path and `fs list events` rows with `Operation` and `Path` fields.
