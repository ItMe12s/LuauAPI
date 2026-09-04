# Testing

## Summary

The project has host tests that run without the game.
They cover paths, the allocator, cache keys, the runtime, and the Python code generator.
This page shows how to run them and where each suite lives.

## Turning tests on

Tests are off by default. Enable them with `LUAUAPI_BUILD_TESTS` after a normal configure.
See [Building from source](building.md).

```bash
cmake -B build -DLUAUAPI_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

CTest discovers the cases right before the test run.
Run one binary with `ctest --test-dir build -R <name>`.

## The C++ tests

Catch2 `v3.15.1`, built into the `luauapi_tests` executable.

| Directory                | Covers                                                                                                                          |
| ------------------------ | ------------------------------------------------------------------------------------------------------------------------------- |
| `tests/cpp/bindings/`    | One file per binding surface: web, websocket, ImGui, task scheduler, mod sandbox, fs paths, Geode task handles, resource bounds |
| `tests/cpp/core/`        | Public API, native registration, runtime lifecycle, `loadstring`, limits alignment, indexed slot map                            |
| `tests/cpp/diagnostics/` | Crash sidecar boundary stack, flush, and serialization                                                                          |
| `tests/cpp/framework/`   | Usertypes, fields, callbacks, containers, handles, deferred release, opaque pointers, views                                     |
| `tests/cpp/require/`     | Require path rules, sandbox containment, cache keys                                                                             |

Host-only suites that compile real subsystems without cocos2d:

| Directory              | Covers                                                                            |
| ---------------------- | --------------------------------------------------------------------------------- |
| `tests/host/lunar/`    | Lunar rig spec, animation compilation, and the built-in editor Luau modules       |
| `tests/host/render3d/` | glTF parse, JPEG magic check, transform math, frustum, procedural mesh, draw list |

The host stub clusters under `tests/host/` (`Geode/`, `arc/`, `framework/`, `cocos2d.h`, `matjson.hpp`) stand in for SDK headers.
`cocos2d.h` retains on `WeakRef` construction like the real pool.

The editor suites in `tests/host/lunar/` run the real `mod/ledit/*.luau` modules against fake `lunar`, `geode.fs`, and `imgui` bindings.
CMake defines `LEDIT_SOURCE_DIR` pointing at `mod/ledit` so `LunarEditorLuauTests.cpp` can read the module sources.
`kFakes` in that file is a fake-Lunar/fake-FS Lua preamble prepended by `EditorEnv::run` before every test body.

Web tests use the in-memory stub in `tests/host/Geode/utils/web.hpp` (no network egress).
`send()` resolves through `geode::utils::web::test::responseFactory`.
WebSocket runtime tests drain `queueInMainThread` on the test main thread between polls.

Deferred from host coverage:

- Live HTTP, TLS, and proxy behavior
- Web listener event injection

## No game libraries in tests

The host tests must not link game libraries (Geode bindings, Cocos2d, extensions, GLEW, FMOD, OpenGL).
They link Catch2, Luau, glm, `luauapi_imgui_headless`, IXWebSocket, mbedTLS, and `ws2_32` on Windows.
See `CMakeLists.txt` for the full list.

Viewport rendering and GPU framebuffer setup are tested in-game only.

## Shared test helpers

C++ helpers live in `tests/host/lua_test_helpers.hpp`:

- `RuntimeGuard` variants (`ModRuntimeGuard`, `BindingModRuntimeGuard`, `ImGuiBindingRuntimeGuard`) reset the runtime per case.
- `ScopedTempDir` removes its directory on scope exit.
- Script helpers such as `runScriptReturnsBool`.

ImGui binding tests drive `tests/host/ImGuiTestHarness.hpp`.

Python codegen fixtures live in `tests/luau_codegen/test_support.py`
(`ROOT`, `DELEGATE_SPECS`, `resolve_test_bindings_dir()`, `all_platforms()`, `types_text()`).
Import `test_support` when a test needs them. Do not add a re-export barrel under `tests/luau_codegen/helpers/`.

## The Python codegen tests

CTest registers the suite as `luauapi_codegen_tests` and runs it via `unittest discover`.

```bash
PYTHONPATH=tools python -m unittest discover -s tests/luau_codegen -p "test_*.py"
```

Tests import from `luau_codegen.*`. Run one area by pattern:

```bash
PYTHONPATH=tools python -m unittest discover -s tests/luau_codegen -p "test_type_map.py"
```

| Package        | Covers                                                                                        |
| -------------- | --------------------------------------------------------------------------------------------- |
| `broma/`       | `.bro` parse, delegate discovery, Geode header scanner                                        |
| `policy/`      | Bindability filtering, platform fields, constructors, hooks, denylist freshness               |
| `typemap/`     | C++ to Lua kinds, enums, value structs, nested and pair containers, `ccCArray`, pointer grids |
| `marshalling/` | Stack checks, callbacks, out-ref multi-return, keyword renames, delegates                     |
| `bindings/`    | Generated C++ handlers, free functions, Luau stub emission, CLI exit codes                    |
| `audit/`       | Skip buckets, parity report, intentional one-offs                                             |
| `guards/`      | Drift guards between stubs, registrars, denylist, and the public header                       |

See [Codegen](codegen/codegen.md) for what the generator produces.

## CI

GitHub Actions workflow `.github/workflows/multi-platform.yml` runs:

| Job            | Platform                                  | What it runs                                                                     |
| -------------- | ----------------------------------------- | -------------------------------------------------------------------------------- |
| `host-tests`   | Windows, macOS                            | Host-only configure (`LUAUAPI_HOST_ONLY=ON`), builds `luauapi_tests`, runs CTest |
| `build` matrix | Windows, macOS, iOS, Android32, Android64 | Full mod build via Geode SDK                                                     |
| `package`      | Ubuntu                                    | Combines matrix artifacts                                                        |

## Related

- [Architecture](architecture.md)
- [Building from source](building.md)
- [Codegen](codegen/codegen.md)
- [Limits and errors](../reference/cpp/limits-and-errors.md)

## Source

- `CMakeLists.txt`
- `cmake/TestSources.cmake`
- `tests/`
