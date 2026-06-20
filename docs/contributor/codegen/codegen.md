# Codegen

## Summary

The binding code generator turns Geode binding files into C++ bindings and Luau type stubs.
It is a Python tool in `tools/luau_codegen/`. This is an overview, not a full reference for every module.

## What it produces

The generator reads Broma binding files for one Geometry Dash version and one platform, then writes:

- C++ binding sources into `build/luauapi-gen/src`.
  These expose game classes to Lua and implement the hook functions.
- One Luau type stub at `types/geode.d.luau`,
  holding all bound classes, factories, enum names, and the root `geode` namespace.
  The `types/` folder is created in the repo root during build and is gitignored.
- Metadata files: a schema, a report, a parity file, and an audit.

The generated C++ is compiled into the main library. It is machine written, so you do not edit it.
The build compiles it with warnings disabled and optimized for size.

## How the build calls it

`CMakeLists.txt` runs `python -m luau_codegen` (the package in `tools/luau_codegen/`, with `tools/` on `PYTHONPATH`)
as the `luauapi_codegen` target, and the main library depends on it.
Configure runs `--list-all-outputs` once so CMake knows the binding and type byproducts.

The stamp command runs codegen in one step:

- Delegate specs are loaded.
- Trampolines are loaded.
- Bindings and stubs are emitted.

The codegen platform comes from the build target.
See `cmake/Codegen.cmake` `luauapi_set_codegen_platform()`.

| Build target | Codegen platform |
| --- | --- |
| Windows | `win` |
| Android arm64-v8a | `android64` |
| Android armeabi-v7a | `android32` |
| iOS | `ios` |
| macOS universal (`arm64` + `x86_64`) | `mac` |
| macOS arm64 only | `m1` |
| macOS x86_64 only | `imac` |

The GD bindings version comes from `mod.json` `gd.<key>`.
The key follows the codegen platform.
Android ABIs use `android`.
Apple desktop platforms use `mac`.
If the key is missing, CMake falls back to `gd.win`, then to `2.2081`.
The Broma tree is `geode_bindings/bindings/<version>/`.
Keep `LUAUAPI_BINDINGS_GIT_TAG` in CMake aligned with the mod.json GD version.

`LUAUAPI_HOST_ONLY=ON` skips the Geode SDK and configure-time listing for host-only tooling.

## Binary size

Bindings make up most of the binary, so the build minimizes them.
A size flag is set on the binding target (`-Oz` on Clang, `-Os` on GCC, `/O1` on MSVC and clang-cl).
On Windows, linker `/OPT:REF` and `/OPT:ICF` drop unused code and fold identical functions.
These apply to non-debug builds only.

## The hook generator

`emit/hooks.py` emits one C++ function per hookable game method. That function:

1. Runs the registered `before` callbacks, pushing `self` and the arguments to Lua.
2. Applies an argument override or a skip, if the callback returned one.
3. Calls the original method, unless it was skipped.
4. Runs the registered `after` callbacks, applying a return override if given.

`emit/cxx_templates.py` emits the shared hook runtime that stores callbacks, sorts them by priority, and calls them.
Invalid overrides are type-checked via pcall. Binding and hook decode share `convert/marshalling.py` `emit_stack_check`.
A method is hookable only when its address is known on the target platform and its arguments and return can be marshalled.
Static methods, constructors, and destructors are skipped.

## Inputs that trigger a rebuild

The custom command depends on the following. A change to any reruns codegen:

- Python sources
- Broma definitions
- Extra stubs in `tools/luau_codegen/extra_bindings/`
- Geode UI headers (`Geode/ui/*.hpp`, discovered via `parse/geode_sdk.py`)
- Free-function headers listed in `model/free_fn_sources.py`

## Geode SDK scan scope

`parse/geode_sdk.py` discovers extra bindings from the installed Geode SDK headers:

- GD and Geode enums (see Geode and GD enums below).
- UI classes and enums from `Geode/ui/*.hpp` headers included by `UI.hpp`.
- Free functions from the manifest in `model/free_fn_sources.py` (`FREE_FUNCTION_SOURCES`).

The manifest is the single source for both the scanner and the generated `#include` list.
`test_binding_guards_framework.py` `FreeFnManifestSyncTests` fails on drift.

| SDK header | Namespaces | Name filter |
| --- | --- | --- |
| `utils/general.hpp` | `geode::utils`, `geode::utils::clipboard`, `geode::utils::game`, `geode::utils::thread`, `geode::utils::platform` | all |
| `ui/Popup.hpp` | `geode` | `createQuickPopup` only |
| `ui/GeodeUI.hpp` | `geode` | all |
| `utils/string.hpp` | `geode::utils::string` | all |
| `utils/random.hpp` | `geode::utils::random` | all |
| `utils/cocos.hpp` | `geode::cocos` | all |

Only `GEODE_DLL` declarations are processed, and functions must be marshallable.
Only the first free function per name and arity is kept.
HTTP, async tasks, and file I/O are not in this manifest.
They live in handwritten bindings with matching extra stub files.

## Geode and GD enums

`parse/geode_sdk.py` reads `bindings/include/Geode/Enums.hpp` and UI enum tables into `CodegenContext`.
`emit/bindings/geode_enums.py` registers integer constant tables under `geode.gd` and `geode`.
`emit/luau_types/enums.py` emits stub types in `types/geode.d.luau`.

Enums marshal as numbers in both directions (`check<int>` in, `lua_pushnumber` out).
For script usage and compares, see [enums](../../reference/lua/enums.md).

## Handwritten type-stub fields

Some bindings are written by hand in `src/bindings/` and `src/framework/`.
Their type signatures still need to be in `types/geode.d.luau`.

Two ways to add them:

- `tools/luau_codegen/extra_bindings/*.dluau`: appended at the end of the stub.
  Current files: `fs.dluau`, `gd3d.dluau`, `hook.dluau`, `imgui.dluau`, `json.dluau`,
  `keyboard.dluau`, `mod.dluau`, `task.dluau`, `web.dluau`, `websocket.dluau`.
  Use this for new globals and for support types the `geode` namespace references.
- `tools/luau_codegen/emit/luau_types/manual_fields.py`: injects fields into a namespace
  that codegen already emits, such as `geode.cocos`.

Handwritten namespaces are two copies of the same surface (the C++ registrar and the stub declaration),
so they must stay in sync. `test_manual_fields_sync.py` and `test_extra_bindings_sync.py` fail CI on drift.

`geode.cocos` is hybrid:

- Codegen emits `geode::cocos` helpers into `bindings_free_functions.cpp`
- `GeodeCocosBinding.cpp` registers only:
  - color helpers
  - `ccDrawColor4B`
  - hex parsers
- `test_binding_guards_*.py` enforces that split

## Metadata outputs

After generation, `build/luauapi-gen/` holds:

- `schema.json`: parsed Broma classes, bound fields, ambiguous overloads,
  `supportedFreeFunctions`, `unscannedGdEnums`, `manualFields`, and `extraBindings`
- `report.md`: counts, skip reason histograms, intersection stats, linkless classes per platform,
  SDK scan warnings, unscanned GD enum aliases, and handwritten stub provenance
- `parity.json`: cross-platform method support matrix (see [Platform parity](platform-parity.md))
- `audit.md`: skip audit grouped by bucket (see Audit buckets below)
- `delegate_specs.py`: generated delegate type specs, installed before binding emit

`emit/metadata.py` writes the schema and report.
`emit/parity.py` writes `parity.json`.
`emit/audit.py` writes the audit.
Run `--audit-report-out` or `--parity-report-out` to regenerate a report without a full stamp.

## Audit buckets

`emit/audit.py` groups skipped methods and free functions into buckets.
Each bucket has a count, a reason histogram, and up to 25 sample skips.

| Bucket id | Category |
| --- | --- |
| `callback_method` | Broma callback methods |
| `sel_arg` | cocos2d selector args |
| `std_function_arg` | `std::function` / `geode::Function` args |
| `callback_alias` | `Callback` alias args |
| `delegate_arg` | delegate pointer args without a bound spec |
| `container_arg` | unsupported `gd` container args |
| `value_type_arg` | FMOD / Kazmath value types |
| `http_async_excluded` | HTTP / async free functions |
| `other` | unclassified skips |

Supported bindings do not appear in these buckets.
See `tests/luau_codegen/test_audit.py` for bucket rules.

## Type classification

`convert/type_classification.py` maps each C++ arg or return to a `TypeInfo` kind.
Container parsing lives in the same module. Normalization helpers live in `convert/type_map.py`.
Binding, hook, delegate, and stub emit all call `classify_arg()` or `classify_return()`.

Resolution order in `_classify_core()`:

1. Out-pointer containers, primitive vectors, std arrays, nested views, and `ccCArray` views
2. `gd` containers and `std::pair`
3. Primitives: `bool`, wide integers as string, numeric, string
4. Value structs from `VALUE_TYPES` in `model/value_types.py` (via `convert/type_map.py`)
5. Enums from `CodegenContext`
6. Opaque handles from `OPAQUE_HANDLE_TYPES`
7. `geode::Result<>` as boolean or string
8. Callbacks (`std::function`, `geode::Function`) on args only
9. cocos2d selectors on args only
10. Delegate pointers on args only, via `model/delegate_specs.py`
11. Object pointers last

`CALLBACK_ALIASES` and `CLASS_CALLBACK_ALIASES` in `type_map.py` expand shorthand names before classification.
Unsupported types return `None`, which becomes a skip reason such as `unsupported-arg:<type>`.

## Overload resolution

Broma can declare several methods with the same name.
The generator groups them by name and input arity (`group_supported()` in `policy/filtering.py`).
When two overloads share a name and input arity, only one can bind, because Lua dispatches on arity.
The generator keeps one only if `PREFERRED_OVERLOADS` lists its normalized arg signature,
otherwise every colliding overload is skipped with `ambiguous-overload-arity:<arity>`.
Ambiguous overloads cause codegen to exit with code 6 when building the emit plan.

One Lua table key per method name. Runtime picks overload by arity only, not arg types.
Stubs widen with `...any` where overloads disagree. See [type stubs](../../reference/lua/type-stubs.md).

## Value struct gate

Cocos value types such as `CCPoint`, `RGBColor`, and `BlendFunc` are always bound.
They live in `model/value_types.py` (single registry for classify, C++ check/push, and Luau stubs).
`convert/type_map.py` re-exports `VALUE_TYPES` and `VALUE_CHECK_CXX_TYPES` derived from that table.

`model/value_struct_gate.py` holds two opt-in lists:

- `DENIED_VALUE_STRUCTS`: explicit deny with a reason (for example `ChanceObject`)
- `GATED_VALUE_STRUCTS`: extra structs that need a custom stub body (for example `SmartPrefabResult`)

Gated names are entries in `model/value_types.py` tagged via `GATED_VALUE_STRUCTS`.
`emit/luau_types/references.py` emits export types from the same registry.
See `tests/luau_codegen/test_value_struct_gate.py`.

## Nullable pointer policy

Return stubs use `?` on object and opaque pointers. Arg stubs do not.
Runtime rejects nil object args unless `allow_nil_object` (hook overrides only).

## Field and container policy

Class fields are filtered in `policy/fields.py` by `bindable_field()`.

A field is skipped when any of the following apply:

- Its type cannot be marshalled
- It is an array or reference
- It is a function or string pointer
- It is listed in `INACCESSIBLE_FIELDS`
- Its type cannot be marshalled as a `SeedValue*` field (see below)

`SeedValue*` fields (for example `geode::SeedValueRSV`) bind as Lua `number`.
Classification uses kind `seed_value`. Getters decode with `static_cast<int>`.
Setters assign with `operator=(int)`, not `static_cast<SeedValue>(int)`.
All eight `SeedValue` variant names share this policy.

Skipped fields appear as `-- skipped <name>: <reason>` comments in the stub.

`bindable_field()` uses the same container gates as methods, except getter-only
container fields (`cc_c_array_view`, `nested_primitive_vector_view`).

`gd::map` and `gd::set` field setters use `assignMap` / `assignSet` (clear plus per-entry insert)
instead of whole-container `operator=`, because Geode gnustl on Android lacks `_Rb_tree::_M_move_assign`.
`gd::vector` primitive field setters use `assignPrimitiveVector`. `std::pair` inside containers is bound.
See [Pair containers](pair-containers.md), [Nested containers](nested-containers.md), and [ccCArray read-only fields](cc-c-array.md).

## Denylist maintenance

`model/denylist.py` is hand-maintained with four structures:

- `INACCESSIBLE_METHODS`
- `INACCESSIBLE_CLASSES`
- `PREFERRED_OVERLOADS`
- `BINDABLE_CONSTRUCTORS`

Entries are conservative. `test_denylist.py` fails if any entry no longer resolves after a GD or SDK bump.

## Constructor binding

Constructors and destructors are rejected in `policy/filtering.py`.
`BINDABLE_CONSTRUCTORS` is a narrow opt-in when `create()` is filtered out and Lua has no other way to construct the type.
Each listed signature becomes a synthetic static `new(...)` factory that calls `autorelease()`.

## CLI exit codes

| Code | Meaning |
| --- | --- |
| 2 | Bad arguments, missing required directories, or Python below 3.11 |
| 3 | No Broma classes found after parsing |
| 4 | I/O error reading inputs or writing outputs |
| 5 | Unexpected exception during emit |
| 6 | Ambiguous overloads remain after building the emit plan |

## Delegates

Delegate pointer args bind only when a spec exists in `delegate_specs.py`.

Collection (`emit/delegates.py` `collect()`):

1. `parse/broma_delegates.py` scans `*.bro` for `*Delegate` and `*Protocol` classes.
   It keeps virtual methods whose args and returns classify.
2. Known cocos delegates merge from `COCOS_DELEGATES` in `emit/delegates.py`.
3. Specs write to `build/luauapi-gen/delegate_specs.py` and install into `model/delegate_specs.py`.
4. `LuaDelegates.gen.hpp` and `LuaDelegates.gen.cpp` emit C++ trampolines that call a Lua table.

`convert/type_classification.py` maps each bound delegate pointer to a Lua table type from the spec.
Methods listed in `SKIP` or `SKIP_METHODS` are omitted.

The main stamp runs delegate collection before bindings.
To run only the delegate step:

```bash
PYTHONPATH=tools python -m luau_codegen --emit-delegates --bindings <bindings-dir> --out <gen-dir>
```

See [delegates](../../reference/lua/delegates.md) for how scripts use delegate tables.

## Related

- [Codegen](../../contributor/codegen/codegen.md)
- [game objects](game-objects.md)
- [Platform parity](platform-parity.md)
- [Pair containers](pair-containers.md)
- [Nested containers](nested-containers.md)
- [CCArray methods](cc-array.md)
- [ccCArray read-only fields](cc-c-array.md)
- [Testing](../testing.md)

## Source

- `cmake/Codegen.cmake`
- `CMakeLists.txt`
- `mod.json`
- `tools/luau_codegen/cli/main.py`
- `tools/luau_codegen/parse/collect.py`
- `tools/luau_codegen/parse/geode_sdk.py`
- `tools/luau_codegen/parse/broma_delegates.py`
- `tools/luau_codegen/model/free_fn_sources.py`
- `tools/luau_codegen/model/value_struct_gate.py`
- `tools/luau_codegen/model/delegate_specs.py`
- `tools/luau_codegen/convert/type_classification.py`
- `tools/luau_codegen/convert/type_map.py`
- `tools/luau_codegen/emit/bindings/geode_enums.py`
- `tools/luau_codegen/emit/luau_types/enums.py`
- `tools/luau_codegen/emit/delegates.py`
- `tools/luau_codegen/emit/metadata.py`
- `tools/luau_codegen/emit/audit.py`
- `tools/luau_codegen/emit/parity.py`
- `tools/luau_codegen/policy/filtering.py`
- `tools/luau_codegen/policy/fields.py`
- `tools/luau_codegen/model/denylist.py`
- `tools/luau_codegen/emit/hooks.py`
- `tools/luau_codegen/emit/cxx_templates.py`
- `tools/luau_codegen/emit/luau_types/`
- `tools/luau_codegen/extra_bindings/`
- `tools/luau_codegen/convert/marshalling.py`
- `tools/luau_codegen/emit/bindings/`
