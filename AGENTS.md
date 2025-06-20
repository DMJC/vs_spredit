# Maintainer Guidelines

## Building the project

### Using Make
1. Ensure dependencies are installed:
   - `g++` with C++20 support
   - `pkg-config`
   - `gtkmm-3.0` development headers
   - `cairomm-1.0` development headers
2. Run `make build` to compile `vegastrike_animation.cpp` and produce the `vs_spredit` binary.
3. Run `make clean` to remove the binary.
4. If a `test.cpp` file exists, run `make test` to compile and execute unit tests.

### Using CMake
This repository does not include a CMake configuration by default. To build with CMake:
1. Create a `CMakeLists.txt` that targets the sources in this repo and sets the standard to C++20.
2. From the repository root run:
   ```bash
   cmake -S . -B build
   cmake --build build
   ```
3. Run tests (if defined) with `ctest --test-dir build`.

## Formatting guidelines
- Use C++20 language features and standard library.
- Prefer `#pragma once` or traditional include guards for header files and avoid mixing the two.
- Indent code with four spaces; do not use tabs.
- Keep lines under 100 characters when possible.

## Running tests and CI
- Local tests can be run via `make test` or `ctest` depending on the chosen build system.
- Continuous integration is configured in `.github/workflows/ci.yml`. The workflow installs dependencies, builds the project with Make, and runs tests if present.
