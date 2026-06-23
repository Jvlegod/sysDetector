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

The current `proc` selftest is a build smoke test for the eBPF proc module. More
runtime tests can be added as separate scripts under `proc/` and called from
`proc/run.sh`.
