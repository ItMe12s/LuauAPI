# Testing

## Summary

The project has host tests that run without the game.
They cover the parts that do not need Cocos2d, such as paths, the allocator, cache keys, and the Python code generator.
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
- `tests/loadstring_tests.cpp` covers `loadstring` compile and runtime behavior.

CTest discovers the Catch2 cases at build time.

## No game libraries in tests

The host tests must not link the game.
`CMakeLists.txt` fails deliberately if the test target tries to link the Geode bindings, Cocos2d, the extensions library, GLEW, FMOD, or OpenGL.
This keeps the tests fast and host only.

## The Python codegen tests

The test suite under `tests/luau_codegen/` covers the Python binding generator:
Broma parsing, filtering, C++ bindings, Luau type stubs, plan/parity, CLI I/O, free-function overrides, and the Geode SDK scanner.
See [Codegen](codegen.md) for what the generator produces.

CTest registers it as `luauapi_codegen_tests` and runs it via `python -m unittest discover`.

Run without CMake:

```bash
PYTHONPATH=tools python -m unittest discover -s tests/luau_codegen -p "test_*.py"
```

| Test file | Coverage |
| --------- | -------- |
| `test_broma.py` | Broma parser, class/method attributes |
| `test_bindings.py` | C++ binding emission, dispatchers, fields |
| `test_codegen_cli.py` | CLI I/O, exit codes, binding root collection |
| `test_filtering.py` | Method eligibility, link classes, denylist |
| `test_free_functions.py` | Free-function platform overrides and emission |
| `test_geode_scanner.py` | Geode SDK header scan warnings |
| `test_hooks.py` | Hook offsets, symbols, hook target emission |
| `test_luau_types.py` | Luau stub layout, factories, overload widening |
| `test_marshalling.py` | Lua stack check/push codegen |
| `test_model.py` | Class lookup, object maps, base resolution |
| `test_parity.py` | Cross-platform parity report |
| `test_plan.py` | Emit plan, intersection, class merge |
| `test_type_map.py` | C++ to Lua type classification |

## Source

- `CMakeLists.txt`
- `tests/allocator_accounting_tests.cpp`
- `tests/bytecode_cache_key_tests.cpp`
- `tests/path_rules_tests.cpp`
- `tests/path_sandbox_tests.cpp`
- `tests/require_path_tests.cpp`
- `tests/loadstring_tests.cpp`
- `tests/luau_codegen/`
