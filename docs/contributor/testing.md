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

The C++ tests use Catch2 `v3.15.1` and build into the `luauapi_tests` executable.

CTest discovers the cases at build time. The files are:

| Test file | Coverage |
| --- | --- |
| `tests/cpp/core/api_tests.cpp` | Public C++ API surface |
| `tests/cpp/framework/binding_tests.cpp` | Binding registration and dispatch |
| `tests/cpp/diagnostics/boundary_recorder_tests.cpp` | Crash sidecar boundary stack, flush, and serialization |
| `tests/cpp/require/bytecode_cache_key_tests.cpp` | Cache key over size, modify time, and content |
| `tests/cpp/framework/callback_tests.cpp` | Callback registration and invocation |
| `tests/cpp/framework/container_tables_tests.cpp` | Recursive container tables and audited pointer grids |
| `tests/cpp/core/config_bounds_tests.cpp` | Resource byte caps stay aligned |
| `tests/cpp/bindings/current_mod_tests.cpp` | Current-mod path helpers against a resources root |
| `tests/cpp/framework/deferred_release_tests.cpp` | Deferred CCObject release queue, shutdown skip, and drain |
| `tests/cpp/framework/fields_tests.cpp` | `m_fields` and release eviction |
| `tests/cpp/require/geode_fs_path_tests.cpp` | Geode filesystem binding path resolution |
| `tests/cpp/framework/handle_gc_tests.cpp` | Task and ImGui draw handle `__gc` cancellation |
| `tests/host/render3d/FrustumTests.cpp` | Frustum culling math |
| `tests/host/render3d/Gd3dMeshBindingTests.cpp` | Procedural mesh Lua binding and handle lifetime |
| `tests/host/render3d/Gd3dTransformBindingTests.cpp` | `gd3d.Transform` constructors and methods |
| `tests/host/render3d/GltfParseTests.cpp` | glTF parse, materials, alpha flags |
| `tests/host/render3d/ImageDecodeTests.cpp` | PNG and JPEG decode limits |
| `tests/host/render3d/ProceduralMeshTests.cpp` | Procedural mesh build and validation |
| `tests/host/render3d/Render3DMathTests.cpp` | Transform math and helpers |
| `tests/host/render3d/SceneDrawListTests.cpp` | Scene draw list sorting and batching |
| `tests/cpp/bindings/imgui_binding_tests.cpp` | ImGui widget binding smoke and font API guards |
| `tests/cpp/bindings/imgui_scheduler_tests.cpp` | ImGui draw scheduler registration |
| `tests/cpp/core/indexed_slot_map_tests.cpp` | Indexed slot map used by the schedulers |
| `tests/cpp/core/loadstring_tests.cpp` | `loadstring` compile and runtime behavior |
| `tests/cpp/framework/misc_correctness_tests.cpp` | Assorted runtime correctness cases |
| `tests/cpp/bindings/mod_fs_isolation_tests.cpp` | Mod filesystem isolation policy |
| `tests/cpp/bindings/mod_sandbox_tests.cpp` | Mod sandbox path resolution and root access policy |
| `tests/cpp/framework/opaque_handle_tests.cpp` | Opaque pointer userdata handles |
| `tests/cpp/require/path_rules_tests.cpp` | Flat path rules and extension checks |
| `tests/cpp/require/path_sandbox_tests.cpp` | File containment and escape rejection |
| `tests/cpp/require/require_path_tests.cpp` | Require child name rules and `.luau` fill in |
| `tests/cpp/require/requirer_root_tests.cpp` | Resources root canonicalization and module resolution |
| `tests/cpp/bindings/resource_bounds_tests.cpp` | Resource size and require bounds |
| `tests/cpp/core/runtime_tests.cpp` | Runtime lifecycle and script execution |
| `tests/cpp/bindings/task_scheduler_tests.cpp` | Task scheduler binding behavior |
| `tests/cpp/framework/usertype_tests.cpp` | Usertype registration, tag limits, and dispatch |
| `tests/cpp/framework/vector_view_tests.cpp` | Readonly vector view lifetime |
| `tests/cpp/bindings/web_binding_tests.cpp` | Geode web binding smoke, caps, sandbox, mock async |
| `tests/cpp/bindings/web_shutdown_tests.cpp` | Web task/listener shutdown and runtime reset |
| `tests/cpp/bindings/websocket_bounds_tests.cpp` | WebSocket scheme, port, payload, and cap guards |
| `tests/cpp/bindings/websocket_runtime_tests.cpp` | WebSocket loopback echo/binary, callbacks, close codes, send bounds |

Host web tests use the in-memory stub in `tests/host/Geode/utils/web.hpp` (no network egress).
The stub implements the Geode web request surface.
`send()` resolves through `geode::utils::web::test::responseFactory` instead of opening sockets.
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

The `luauapi_tests` target links:

- Catch2
- Luau libraries
- glm
- luauapi_imgui_headless
- IXWebSocket
- mbedTLS (`mbedtls`, `mbedx509`, `mbedcrypto`)
- `ws2_32` on Windows

Which keeps the tests fast and host only.

The host test binary compiles part of the 3D stack for parse, math, and binding coverage.
It links gd3d transform, mesh, and glTF binding translation units, but not OpenGL, cocos2d, or `gd3d.ViewportFrame`.
Viewport rendering and GPU framebuffer setup are tested in-game only.

## Shared test helpers

C++ tests share setup in `tests/host/lua_test_helpers.hpp`.

- `RuntimeGuard` and variants reset the runtime and related subsystems after each case.
  Examples include `ModRuntimeGuard`, `BindingModRuntimeGuard`, and `ImGuiBindingRuntimeGuard`.
- `ScopedTempDir` creates a temp directory and removes it on scope exit.
- Script helpers compile and run Luau snippets, such as `runScriptReturnsBool` and `runScriptReturnsString`.

Python codegen tests import `luau_codegen` modules directly.
Shared fixtures live in `tests/luau_codegen/test_support.py`:

- `ROOT` and `DELEGATE_SPECS`
- `resolve_test_bindings_dir()` for Broma fixture lookup
- `all_platforms()` and `types_text()` for emit tests

Import `test_support` when a test needs those fixtures.
Do not add a re-export barrel under `tests/luau_codegen/helpers/`.

## CI

GitHub Actions workflow `.github/workflows/multi-platform.yml` runs:

| Job | Platform | What it runs |
| --- | --- | --- |
| `host-tests` | Windows, macOS | Host-only configure (`LUAUAPI_HOST_ONLY=ON`), builds `luauapi_tests`, runs CTest (Catch2 cases plus `luauapi_codegen_tests`) |
| `build` matrix | Windows, macOS, iOS, Android32, Android64 | Full mod build via Geode SDK |
| `package` | Ubuntu | Combines matrix artifacts |

The host test target links IXWebSocket and mbedTLS for websocket runtime tests.
Windows also links `ws2_32`.
See `CMakeLists.txt` for the full link list.

## The Python codegen tests

CTest registers the suite as `luauapi_codegen_tests` and runs it via `python -m unittest discover`.

```bash
PYTHONPATH=tools python -m unittest discover -s tests/luau_codegen -p "test_*.py"
```

Tests under `tests/luau_codegen/` import from `luau_codegen.*`.
Import `test_support` for shared fixtures such as `ROOT` and `resolve_test_bindings_dir()` (listed above).

Compact map of `tests/luau_codegen/`:

| Area | Test files | Coverage |
| --- | --- | --- |
| Broma parsing | `broma/test_broma.py`, `broma/test_broma_delegates.py`, `broma/test_model.py` | `.bro` parse, link attrs, delegate class discovery |
| Filtering and plan | `policy/test_filtering.py`, `policy/test_plan.py`, `policy/test_constructors.py`, `policy/test_denylist.py` | bindability, platform fields, constructors, denylist freshness |
| Type map | `typemap/test_type_map.py`, `typemap/test_type_map_modules.py`, `typemap/test_value_struct_gate.py`, `typemap/test_cocos_enums.py` | C++ to Lua kinds, enums, value struct gate |
| Containers | `typemap/test_nested_containers.py`, `typemap/test_pair_design.py`, `typemap/test_cc_c_array.py`, `typemap/test_object_vector_audit.py`, `typemap/test_gj_grid_fields.py` | `gd` containers, pairs, `ccCArray`, object vectors, GJ grid fields |
| Marshalling | `marshalling/test_marshalling.py`, `marshalling/test_out_ref_policy.py` | stack checks, callbacks, FMOD, containers, out-ref multi-return |
| Audit one-offs | `audit/test_one_offs.py`, `audit/test_fmod_surface.py` | intentionally skipped methods, FMOD handle rename and value structs |
| Bindings emit | `bindings/test_bindings_handlers.py`, `bindings/test_bindings_overloads.py`, `bindings/test_bindings_safety.py`, `bindings/test_bindings_fmod.py`, `bindings/test_free_functions.py` | generated C++ handlers, free functions, and crash sidecar boundary records |
| Hooks | `policy/test_hooks.py` | hook runtime, offsets, SEL callbacks |
| Luau stubs | `bindings/test_luau_types.py`, `bindings/test_types_binding.py`, `bindings/test_geode_enums_emit.py` | stub emission and cocos value descriptors |
| Geode SDK scan | `broma/test_geode_scanner.py`, `broma/test_geode_ccnode.py`, `broma/test_geode_ccarray.py`, `broma/test_cpp_scan.py` | header scanner and cocos additions |
| Delegates | `marshalling/test_delegate_generator.py` | delegate spec and trampoline emit |
| Parity and audit | `audit/test_parity.py`, `audit/test_audit.py` | `parity.json` and audit bucket rules |
| CLI | `bindings/test_codegen_cli.py` | exit codes, list-all-outputs, standalone audit |
| Drift guards | `guards/test_manual_fields_sync.py`, `guards/test_extra_bindings_sync.py`, `guards/test_public_api_header_sync.py`, `guards/test_binding_guards_*.py` | stub, binding, and public header sync |

See [Codegen](codegen/codegen.md) for what the generator produces.

## Related

- [Architecture](architecture.md)
- [Building from source](building.md)
- [Codegen](codegen/codegen.md)
- [Limits and errors](../reference/cpp/limits-and-errors.md)

## Source

- `CMakeLists.txt`
- `tests/cpp/`
- `tests/host/`
- `tests/host/lua_test_helpers.hpp`
- `tests/luau_codegen/`
- `tests/luau_codegen/test_support.py`
