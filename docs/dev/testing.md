# Testing

## Summary

The project has host tests that run without the game.
They cover the parts that do not need Cocos2d, such as paths, the allocator, cache keys, and the hook generator.
This page lists them and shows how to run them.

## Turning tests on

Tests are disabled by default. Enable them with the `LUAUAPI_BUILD_TESTS` option, then build and run with CTest.

```bash
cmake -B build -DLUAUAPI_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

## The C++ tests

The C++ tests use Catch2 `v3.15.0` and build into the `luauapi_tests` executable. The files are:

- `tests/allocator_accounting_tests.cpp` covers the bounded allocator math and limits.
- `tests/bytecode_cache_key_tests.cpp` covers the cache key, including changes to size, modify time, and content.
- `tests/path_rules_tests.cpp` covers flat path rules and extension checks.
- `tests/path_sandbox_tests.cpp` covers file containment and escape rejection.
- `tests/require_path_tests.cpp` covers require child name rules and the `.luau` extension fill in.

CTest discovers the Catch2 cases at build time.

## No game libraries in tests

The host tests must not link the game.
`CMakeLists.txt` fails deliberately if the test target tries to link the Geode bindings, Cocos2d, the extensions library, GLEW, FMOD, or OpenGL.
This keeps the tests fast and host only.

## The Python test

`tests/luau_codegen_hook_tests.py` checks the hook generator. CTest registers it as `luauapi_codegen_hooks` and runs it with Python.
It is a large test that covers offsets, symbols, platform intersection, argument marshalling, and return decoding.

## Source

- `CMakeLists.txt`
- `tests/allocator_accounting_tests.cpp`
- `tests/bytecode_cache_key_tests.cpp`
- `tests/path_rules_tests.cpp`
- `tests/path_sandbox_tests.cpp`
- `tests/require_path_tests.cpp`
- `tests/luau_codegen_hook_tests.py`
