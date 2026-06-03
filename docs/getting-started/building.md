# Building

## Summary

This page explains how to build the mod and how the binding code generator fits into the build.

## Build

See [Installation](installation.md) for prerequisites.

```bash
cmake -B build
cmake --build build
```

The first `cmake -B build` also runs codegen listing at configure time. Python 3.11+ must be on PATH before configure.
For platform builds, use the Geode CLI or the CI workflow in `.github/workflows/multi-platform.yml`.

## Code generation

The build runs the `luauapi_codegen` target before compiling the main library.
It reads Broma binding files and writes generated C++ plus `types/geode.d.luau`.
Do not edit generated files by hand. Change the generator or its inputs and rebuild.
See [Codegen](../dev/codegen.md) for outputs, the configure and stamp commands, and platform mapping.

## Building and running tests

Tests are disabled by default. Enable them with `-DLUAUAPI_BUILD_TESTS=ON`, build, then run CTest.
See [Testing](../dev/testing.md) for the C++ and Python suites and how to run codegen tests without CMake.

## Source

- `CMakeLists.txt`
- `tools/luau_codegen/cli/main.py`
