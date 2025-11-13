# Changelog
All notable changes to this project will be documented in this file. The format is based on [***Keep a Changelog***](https://keepachangelog.com/en/1.0.0/).

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
