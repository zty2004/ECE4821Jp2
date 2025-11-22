# Changelog
All notable changes to this project will be documented in this file. The format is based on [***Keep a Changelog***](https://keepachangelog.com/en/1.0.0/).

## [m3] - 2025-11-23

### Added

- support `LISTEN` query for file-based query input
- support `--listen <filename>` command-line argument
- support `--threads=<int>` command-line argument with auto-detection (0 = auto-detect)
- support dependency-aware task scheduling with TaskQueue
- support priority queue for task distribution complexity
- comprehensive wiki documentation (architecture, multi-threading, performance)
- README with complete build and usage instructions

### Changed

- optimize thread pool with configurable thread count
- enhance reader-writer locks with fine-grained locking strategy
- improve synchronization primitives for critical sections
- refine query ID assignment to continuous integer sequence

### Fixed

- resolve race conditions in concurrent query execution
- fix deadlock scenarios in lock acquisition
- handle LISTEN file errors gracefully
- improve error handling for command-line arguments

## [m2] - 2025-11-13

### Added

- support pop-up thread executor
- support lock manager
- support database layer architecture

### Changed

- switch from single thread to multiple thread

### Fixed

- fix and improve clang-tidy
- fix and improve cppcheck
- fix and improve cpplint

## [m1] - 2025-10-26

### Added

- support gitea workflows
- support aggregate queries `SUM`, `COUNT`.
- support arithmatic queries `MIN`, `MAX`, `ADD`, `SUB`.
- support data manipulation queries `SELECT`, `DELETE`, `SWAP`, `DUPLICATE`.
- support table management `COPYTABLE`, `TRUNCATE`.
