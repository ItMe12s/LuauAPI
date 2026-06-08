# Testing

## Summary

The project has host tests that run without the game.
They cover the parts that do not need Cocos2d, such as paths, the allocator, cache keys, and the Python code generator.
This page lists them and shows how to run them.

## Turning tests on

Tests are disabled by default. Enable them with the `LUAUAPI_BUILD_TESTS` option after a normal configure.
See [Building](../getting-started/chapter-2.md) for the base CMake workflow.

```bash
cmake -B build -DLUAUAPI_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

## The C++ tests

The C++ tests use Catch2 `v3.15.0` and build into the `luauapi_tests` executable. The files are:

| Test file | Coverage |
| --- | --- |
| `tests/allocator_accounting_tests.cpp` | Bounded allocator math and limits |
| `tests/api_tests.cpp` | Public C++ API surface |
| `tests/binding_tests.cpp` | Binding registration and dispatch |
| `tests/bytecode_cache_accounting_tests.cpp` | Bytecode cache accounting limits |
| `tests/bytecode_cache_key_tests.cpp` | Cache key, including changes to size, modify time, and content |
| `tests/callback_tests.cpp` | Callback registration and invocation |
| `tests/config_bounds_tests.cpp` | Resource byte caps stay aligned (web/fs read/write) |
| `tests/current_mod_tests.cpp` | Current-mod path helpers against a resources root |
| `tests/fields_tests.cpp` | `m_fields` and release eviction |
| `tests/geode_fs_path_tests.cpp` | Geode filesystem binding path resolution inside a root |
| `tests/handle_gc_tests.cpp` | Task handle garbage collection cancellation |
| `tests/imgui_scheduler_tests.cpp` | ImGui draw scheduler registration |
| `tests/indexed_slot_map_tests.cpp` | Indexed slot map structure used by the schedulers |
| `tests/loadstring_tests.cpp` | `loadstring` compile and runtime behavior |
| `tests/misc_correctness_tests.cpp` | Assorted runtime correctness cases |
| `tests/mod_sandbox_tests.cpp` | Mod sandbox path resolution and root access policy |
| `tests/opaque_handle_tests.cpp` | Opaque pointer userdata handles |
| `tests/path_rules_tests.cpp` | Flat path rules and extension checks |
| `tests/path_sandbox_tests.cpp` | File containment, virtual chunk paths, and escape rejection |
| `tests/require_path_tests.cpp` | Require child name rules and the `.luau` extension fill in |
| `tests/requirer_root_tests.cpp` | Resources root canonicalization and module resolution |
| `tests/resource_bounds_tests.cpp` | Resource size and require bounds |
| `tests/runtime_tests.cpp` | Runtime lifecycle and script execution |
| `tests/task_scheduler_tests.cpp` | Task scheduler binding behavior |
| `tests/usertype_tests.cpp` | Usertype registration, tag limits, and method dispatch |
| `tests/vector_view_tests.cpp` | Readonly vector view lifetime |

CTest discovers the Catch2 cases at build time.

## No game libraries in tests

The host tests must not link the game: no Geode bindings, Cocos2d, extensions library, GLEW, FMOD, or OpenGL.
The `luauapi_tests` target links only Catch2, fmt, and the Luau libraries, which keeps the tests fast and host only.

## The Python codegen tests

The test suite under `tests/luau_codegen/` covers the Python binding generator:

- Broma parsing
- Filtering
- C++ bindings
- Luau type stubs
- Plan/parity
- CLI I/O
- Free-function overrides
- Geode SDK scanner
- Drift guards for handwritten bindings, extra-binding stubs, and the host public API header

See [Codegen](codegen.md) for what the generator produces.

CTest registers it as `luauapi_codegen_tests` and runs it via `python -m unittest discover`.

Run without CMake:

```bash
PYTHONPATH=tools python -m unittest discover -s tests/luau_codegen -p "test_*.py"
```

| Test file | Coverage |
| --- | --- |
| `test_audit.py` | Skip audit buckets, markdown sections, totals vs plan |
| `test_binding_guards.py` | Web/fs size-cap enforcement, callback logging, handle `__gc`, cocos hybrid, task-tick, free-fn guards |
| `test_bindings_fmod.py` | FMOD opaque handle bindings emit tagged opaque userdata |
| `test_bindings_handlers.py` | SEL/menu handler collapse, schedule, lazy sprite callbacks |
| `test_bindings_overloads.py` | Ambiguous overload policy, preferred overloads, const mangling |
| `test_bindings_safety.py` | Generated hooks, fields, containers, hook API safety, codegen report notes |
| `test_broma.py` | Broma parser, class/method attributes |
| `test_broma_delegates.py` | Delegate parser edge cases and pointer setter discovery |
| `test_cc_c_array.py` | ccCArray allowlist, type map, marshalling, readonly fields |
| `test_codegen_cli.py` | CLI I/O, exit codes, binding root collection |
| `test_cpp_scan.py` | C++ scanner delimiter and template helpers |
| `test_constructors.py` | `BINDABLE_CONSTRUCTORS` opt-in, `new` factory emission |
| `test_delegate_generator.py` | Delegate spec collection, C++ trampoline emission |
| `test_denylist.py` | Denylist and preferred-overload entry validity |
| `test_extra_bindings_sync.py` | `extra_bindings/{mod,fs,json,web}.dluau` vs handwritten C++ registration |
| `test_filtering.py` | Method eligibility, link classes, denylist |
| `test_free_functions.py` | Free-function platform overrides and emission |
| `test_geode_ccnode.py` | Geode `CCNode` SDK scan merge, bindings, anchor enums |
| `test_geode_scanner.py` | Geode SDK header scan warnings |
| `test_hooks.py` | Hook offsets, symbols, null guards, protected override calls |
| `test_luau_types.py` | Luau stub layout, factories, overload widening |
| `test_manual_fields_sync.py` | `manual_fields.py` vs handwritten Geode namespace C++ registration |
| `test_marshalling.py` | Lua stack check/push codegen |
| `test_model.py` | Class lookup, object maps, base resolution |
| `test_nested_containers.py` | Nested container policy, type map, marshalling, fields |
| `test_object_vector_audit.py` | Object vector audit classification and opaque handles |
| `test_pair_design.py` | Pair record shapes, pair-key maps, baseline field policy |
| `test_parity.py` | Cross-platform parity report |
| `test_plan.py` | Emit plan, intersection, class merge |
| `test_public_api_header_sync.py` | `include/LuauAPI.hpp` vs host test header, minus intentional async omissions |
| `test_type_map.py` | C++ to Lua type classification |
| `test_type_map_modules.py` | Type-map module split facade compatibility |
| `test_value_struct_gate.py` | Gated value struct denylist and stub emission |

## Source

- `CMakeLists.txt`
- `tests/*.cpp`
- `tests/host/`
- `tests/luau_codegen/`
