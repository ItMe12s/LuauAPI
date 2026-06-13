# Testing

## Summary

The project has host tests that run without the game.
They cover the parts that do not need Cocos2d,
such as paths, the allocator, cache keys, and the Python code generator.
This page lists them and shows how to run them.

## Turning tests on

Tests are off by default. Enable them with `LUAUAPI_BUILD_TESTS` after a normal configure.
See [Building from source](building.md).

```bash
cmake -B build -DLUAUAPI_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

## The C++ tests

The C++ tests use Catch2 `v3.15.0` and build into the `luauapi_tests` executable.

CTest discovers the cases at build time. The files are:

| Test file | Coverage |
| --- | --- |
| `tests/allocator_accounting_tests.cpp` | Bounded allocator math and limits |
| `tests/api_tests.cpp` | Public C++ API surface |
| `tests/binding_tests.cpp` | Binding registration and dispatch |
| `tests/bytecode_cache_accounting_tests.cpp` | Bytecode cache accounting limits |
| `tests/bytecode_cache_key_tests.cpp` | Cache key over size, modify time, and content |
| `tests/callback_tests.cpp` | Callback registration and invocation |
| `tests/config_bounds_tests.cpp` | Resource byte caps stay aligned |
| `tests/current_mod_tests.cpp` | Current-mod path helpers against a resources root |
| `tests/fields_tests.cpp` | `m_fields` and release eviction |
| `tests/geode_fs_path_tests.cpp` | Geode filesystem binding path resolution |
| `tests/handle_gc_tests.cpp` | Task and ImGui draw handle `__gc` cancellation |
| `tests/host/FrustumTests.cpp` | Frustum culling math |
| `tests/host/Gd3dMeshBindingTests.cpp` | Procedural mesh Lua binding and handle lifetime |
| `tests/host/Gd3dTransformBindingTests.cpp` | `gd3d.Transform` constructors and methods |
| `tests/host/GltfParseTests.cpp` | glTF parse, materials, alpha flags |
| `tests/host/ImageDecodeTests.cpp` | PNG and JPEG decode limits |
| `tests/host/ProceduralMeshTests.cpp` | Procedural mesh build and validation |
| `tests/host/Render3DMathTests.cpp` | Transform math and helpers |
| `tests/host/SceneDrawListTests.cpp` | Scene draw list sorting and batching |
| `tests/imgui_scheduler_tests.cpp` | ImGui draw scheduler registration |
| `tests/indexed_slot_map_tests.cpp` | Indexed slot map used by the schedulers |
| `tests/loadstring_tests.cpp` | `loadstring` compile and runtime behavior |
| `tests/misc_correctness_tests.cpp` | Assorted runtime correctness cases |
| `tests/mod_sandbox_tests.cpp` | Mod sandbox path resolution and root access policy |
| `tests/opaque_handle_tests.cpp` | Opaque pointer userdata handles |
| `tests/path_rules_tests.cpp` | Flat path rules and extension checks |
| `tests/path_sandbox_tests.cpp` | File containment and escape rejection |
| `tests/require_path_tests.cpp` | Require child name rules and `.luau` fill in |
| `tests/requirer_root_tests.cpp` | Resources root canonicalization and module resolution |
| `tests/resource_bounds_tests.cpp` | Resource size and require bounds |
| `tests/runtime_tests.cpp` | Runtime lifecycle and script execution |
| `tests/task_scheduler_tests.cpp` | Task scheduler binding behavior |
| `tests/usertype_tests.cpp` | Usertype registration, tag limits, and dispatch |
| `tests/vector_view_tests.cpp` | Readonly vector view lifetime |
| `tests/web_binding_tests.cpp` | Geode web binding smoke, caps, sandbox, mock async |
| `tests/web_shutdown_tests.cpp` | Web task/listener shutdown and runtime reset |
| `tests/websocket_bounds_tests.cpp` | WebSocket scheme, port, payload, and cap guards |
| `tests/websocket_runtime_tests.cpp` | WebSocket loopback echo/binary, callbacks, close codes, send bounds |

Host web tests use the in-memory stub in `tests/host/Geode/utils/web.hpp` (no network egress).
Host websocket runtime tests drain `queueInMainThread` on the test main thread between polls.

Deferred from host coverage:

- Live HTTP, TLS, and proxy behavior
- Web listener event injection

## No game libraries in tests

The host tests must not link the following game libraries:

- Geode bindings
- Cocos2d
- extensions
- GLEW
- FMOD
- OpenGL

The `luauapi_tests` target links only:

- Catch2
- fmt
- Luau libraries

Which keeps the tests fast and host only.

The host test binary compiles part of the 3D stack for parse, math, and binding coverage.
It links gd3d transform, mesh, and glTF binding translation units, but not OpenGL, cocos2d, or `gd3d.ViewportFrame`.
Viewport rendering and GPU framebuffer setup are tested in-game only.

## CI

GitHub Actions workflow `.github/workflows/multi-platform.yml` runs:

| Job | Platform | What it runs |
| --- | --- | --- |
| `windows-tests` | Windows | Builds `luauapi_tests`, runs CTest (Catch2 host suite) |
| `macos-tests` | macOS | Same as Windows |
| `linux-codegen-tests` | Linux | Python codegen tests only (`luauapi_codegen_tests`) |
| `build` matrix | Windows, macOS, iOS, Android32, Android64 | Full mod build via Geode SDK |
| `package` | Ubuntu | Combines matrix artifacts |

The host test target links IXWebSocket and mbedTLS for websocket runtime tests.
Windows also links `ws2_32`.
See `CMakeLists.txt` for the full link list.

## The Python codegen tests

The suite under `tests/luau_codegen/` covers:

- Broma parsing
- Filtering
- C++ bindings
- Luau type stubs
- Plan and parity
- CLI I/O
- Free-function overrides
- The Geode SDK scanner
- Drift guards for handwritten bindings, extra-binding stubs, and the host public API header

CTest registers it as `luauapi_codegen_tests` and runs it via `python -m unittest discover`.

```bash
PYTHONPATH=tools python -m unittest discover -s tests/luau_codegen -p "test_*.py"
```

Notable drift guards: `test_manual_fields_sync.py`, `test_extra_bindings_sync.py`,
`test_public_api_header_sync.py`, and `test_binding_guards.py`.
They prove the stub, handwritten bindings, and public header stay in sync, so docs sourced from them stay accurate.
See [Codegen](codegen/codegen.md) for what the generator produces.

## Related

- [Building from source](building.md)
- [Codegen](codegen/codegen.md)

## Source

- `CMakeLists.txt`
- `tests/*.cpp`
- `tests/host/`
- `tests/luau_codegen/`
