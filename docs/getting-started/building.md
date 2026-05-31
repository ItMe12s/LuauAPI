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

The build runs a Python code generator before it compiles.
This step is the `luauapi_codegen` target, and the main library depends on it.

The generator reads Broma binding files from the pinned Geode bindings checkout and produces two kinds of output:

- C++ binding sources in `build/luauapi-gen/src`
- A Luau type stub, `types/geode.d.luau`

Platform mapping and Geometry Dash version selection are described in [Codegen](../dev/codegen.md).
The generated C++ sources are compiled with warnings disabled, because they are machine written.

The generator entry point is `tools/luau_codegen/codegen.py`. The build calls it three times.
Two of those calls list the expected outputs so that CMake knows the byproducts.
The third call performs the real generation and writes a stamp file.

Generated files are not meant to be edited by hand. To change the generated output, change the generator or its binding inputs and rebuild.
See [Codegen](../dev/codegen.md).

## Building and running tests

Tests are disabled by default. Enable them with the `LUAUAPI_BUILD_TESTS` option.

```bash
cmake -B build -DLUAUAPI_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

This builds the `luauapi_tests` executable and registers a Python test. The C++ tests use Catch2.
The host tests never link the game libraries.
CMake fails deliberately if a test target tries to link Cocos2d, the Geode bindings, or similar game only libraries.
See [Testing](../dev/testing.md).

## Source

- `CMakeLists.txt`
- `tools/luau_codegen/codegen.py`
