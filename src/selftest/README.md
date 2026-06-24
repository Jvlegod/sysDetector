## Selftest

Selftests are grouped by module. Each module owns a subdirectory and exposes a
`run.sh` script.

```bash
# Run all module selftests
./src/selftest/run.sh

# Run one module
./src/selftest/run.sh proc
./src/selftest/run.sh fs
```

### Layout

- `proc/` - process monitoring and eBPF proc module tests
- `fs/` - filesystem monitoring tests

### proc tests

`proc/run.sh` executes the scripts under `proc/tests/`:

- `build.sh` builds the eBPF proc module and checks the `proc` binary exists.
- `config.sh` validates proc configuration files and fixtures contain the
  required unique keys.
- `runtime-events.sh` is a root-gated runtime test. It starts the proc worker,
  server, and CLI, enables a test config, triggers configured and unconfigured
  short-lived commands, and verifies only configured eBPF `EXEC`/`EXIT` events
  appear in `/var/log/sysDetector/proc.log`. When not run as root, it reports
  `SKIP`.
