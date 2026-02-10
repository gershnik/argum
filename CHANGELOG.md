# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),

## Unreleased

### Added
- Help/usage output now supports colors, if desired.
- New `detect-system.h` header for detection of terminal width and whether to 
  colorize output. 
- New `width` argument for `formatUsage`/`formatHelp` to specify desired with.
  (previously this required advanced `HelpFormatter` usage)
- `clang-cl` compiler is now supported on Windows

### Changed
- The `formatUsage`/`formatHelp` is not word wrapped by default. Use
  the aforementioned `width` argument to set specific width.

### Fixed
- Formatted output now better supports non-ascii symbols
- tests are now more robust w.r.t. compiler/environment differences
- module file now works with Visual Studio 2026

## [2.6] - 2025-02-12

### Added
- `ARGUM_NO_TESTS` CMake configuration option. Setting it to `ON` disables all test targets

### Changed
- Test targets are now excluded from default (aka 'all') CMake build
- Test infrastructure changed to Doctest from Catch2

### Fixed
- Added missing `<limits.h>` include file in one of the headers.

## [2.5] - 2023-07-23

### Changed
- Updated CMake configuration to modernize it and allow local installation

## [2.4] - 2023-01-05

### Fixed
- Fixes to C++ module interface
  With MSVC recent bug fixes module now finally works again in 17.3

## [2.3] - 2022-09-05

### Fixed
- Workarounds for bugs in Visual Studio 2022, 17.3.3

## [2.2] - 2022-05-17

### Added
- Support for GCC 12

## [2.1] - 2022-04-04

### Changed
- It is now possible to require an argument to be "attached" to an option.
  That is, for options `-f` and `--foo` to require syntax `-fARG` and `--foo=ARG` and disallow `-f ARG` and `--foo ARG`.
  This can be handy if an argument is optional and you also have positional arguments. 
  See `Option::requireAttachedArgument` method.


## [2.0] - 2022-03-31

### Added

- You can now chose between using exception or expected values for error reporting. Define `ARGUM_USE_EXPECTED` to switch to expected values.
- Building with exceptions (and RTTI) disabled is supported too. This automatically implies `ARGUM_USE_EXPECTED`. On Clang, GCC and MSVC this mode is automatically detected when exceptions are disabled. On other compilers define `ARGUM_NO_THROW`.
- You can  customize the "terminate" function called when Argum encounters illegal arguments or usage. Define `ARGUM_CUSTOM_TERMINATE` and implement `[[noreturn]] inline void Argum::terminateApplication(const char * message)`. See README or Wiki for more details.

### Fixed

- Multiple minor bug fixes 


### Changed
- Updated CMake configuration to modernize it and allow local installation

## [1.3] - 2022-03-19

### Added
- Support for help and usage formatting for subcommands

### Changed
- Better naming in parser configuration

### Changed
- Updated documentation

## [1.2] - 2022-03-14

### Changed
- Better modularization of help formatter

### Fixed
- Various minor bug fixes


## [1.1] - 2022-03-09

### Added

- Added convenience parsers that allow easy conversion and validation of options arguments and positionals
  * Integral parser function: `parseIntegral<T>`
  * Floating point parsing function: `parseFloatingPoint<T>`
  * Choice parser: `ChoiceParser`/`WChoiceParser`. Allows restricting input to a number of choices and returns index of the choice. 
  * Boolean parser: `BooleanParser`/`WBooleanParser`. Based on choice parser and converts things like "1"/"0", "on"/"off", "true"/"false" to boolean `true` or `false`

  If you are not using single-header/module distributions these live in `<argum/type-parsers.h>`

## [1.0] - 2022-03-07

### Added
- Initial version

[1.0]: https://github.com/gershnik/argum/releases/v1.0
[1.1]: https://github.com/gershnik/argum/releases/v1.1
[1.2]: https://github.com/gershnik/argum/releases/v1.2
[1.3]: https://github.com/gershnik/argum/releases/v1.3
[2.0]: https://github.com/gershnik/argum/releases/v2.0
[2.1]: https://github.com/gershnik/argum/releases/v2.1
[2.2]: https://github.com/gershnik/argum/releases/v2.2
[2.3]: https://github.com/gershnik/argum/releases/v2.3
[2.4]: https://github.com/gershnik/argum/releases/v2.4
[2.5]: https://github.com/gershnik/argum/releases/v2.5
[2.6]: https://github.com/gershnik/argum/releases/v2.6
