# Chapter 2: Building

## Summary

This page shows how to build the mod and how the binding code generator fits in.

## Build

See [Chapter 1: Installation](chapter-1.md) for prerequisites.

```bash
cmake -B build
cmake --build build
```

Two shortcuts do the same thing:

- `geode build` runs the Geode CLI, which configures, builds, and installs the mod.
- In VSCode, open the Command Palette (Ctrl+Shift+P or F1), run **CMake: Configure**, then
  **CMake: Build** (CMake Tools extension).

The first configure also runs the codegen listing. Python 3.11+ must be on PATH before you configure.
For platform builds, use the Geode CLI or the CI workflow in `.github/workflows/multi-platform.yml`.

## Code generation

The build runs the `luauapi_codegen` target before it compiles the main library.
It reads Broma binding files and writes generated C++ plus `types/geode.d.luau`.
Do not edit generated files by hand. Change the generator or its inputs, then rebuild.
See [Codegen](../dev/codegen.md) for outputs, the configure and stamp commands, and platform mapping.

## Building and running tests

Tests are off by default. Turn them on with `-DLUAUAPI_BUILD_TESTS=ON`, build, then run CTest.
See [Testing](../dev/testing.md) for the C++ and Python suites and how to run codegen tests without CMake.

## Next

- [Chapter 3: Editor setup](chapter-3.md)

Back to [Chapter 1: Installation](chapter-1.md).

## Source

- `CMakeLists.txt`
- `tools/luau_codegen/cli/main.py`
