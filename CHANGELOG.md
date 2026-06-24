# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.2] - 2026-06-24

### Added

- GitHub Actions release workflow that builds Debian and npm packages from a synchronized version and publishes GitHub release assets.
- Proc runtime event tracking for configured process `exec`, `exit`, `argv`, short-lived process anomalies, suspicious shell command anomalies, and forked child inheritance for daemon-style processes.
- `proc list tracked` support for active configured process instances with PID, PPID, command, executable path, runtime, and argv fields.
- Proc selftests and documentation for validating config-scoped tracking and long-lived tracked process visibility.
- Source build artifact cleanup during uninstall via `clean_src_directories`.

### Changed

- Proc eBPF tracking is restricted to enabled configured targets instead of global arbitrary process events.
- Install and CMake build flows now detect available clang versions such as `clang-18` for eBPF builds.
- eBPF Makefile dependencies now rebuild the proc binary when user-space or BPF source changes.

### Fixed

- Removed noisy unconfigured process exit logs such as `without matching exec event`.
- Fixed install/build mismatch where stale tracked `proc` binaries could be installed after source changes.
- Improved proc tracked semantics for daemonized services by inheriting tracking across forked child processes.

## [0.1.0] - 2025-04-05

### Added

- Initial project directory and baseline sysDetector implementation.
