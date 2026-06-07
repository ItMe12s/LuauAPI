# Codegen

## Summary

The binding code generator turns Geode binding files into C++ bindings and Luau type stubs. It is a Python tool in `tools/luau_codegen/`.
Overview of the binding generator, not a full reference for every internal module.

## What it produces

The generator reads Broma binding files for one Geometry Dash version and one platform. It writes:

- C++ binding sources into `build/luauapi-gen/src`. These expose game classes to Lua and implement the hook functions.
- A single Luau type stub, `types/geode.d.luau`, holding the bound classes, the per-namespace factories, the enum aliases, and the `geode` namespace root. It gives editors autocomplete and type checks.
- Metadata files, including a schema, a report, and a parity file.

The generated C++ is compiled into the main library. Because it is machine written, you do not edit it.
The build compiles it with warnings disabled and optimized for size, since the bindings are large and not performance critical.

## How the build calls it

`CMakeLists.txt` runs `python -m luau_codegen` (the package in `tools/luau_codegen/`,
with `tools/` on `PYTHONPATH`) as the `luauapi_codegen` target, and the main library depends on this target.
Configure runs `--list-all-outputs` once so CMake knows binding and type byproducts.
The stamp command runs codegen in one step: delegate specs and trampolines are loaded before emitting bindings and stubs.

The platform is chosen from the build target. Windows maps to `win`, for example, and an Android 64-bit build maps to `android64`.
The Geometry Dash version is read from the `"win"` entry in `mod.json`.

CMake options:

- `LUAUAPI_HOST_ONLY=ON` skips Geode SDK and configure-time codegen listing. Use for host-only tooling without `GEODE_SDK`.

## Binary size

Bindings make up most of the binary size, so the build minimizes it using two methods:

- A size flag is set on the binding target so all bindings compile for minimal size. This is `-Oz` on Clang, `-Os` on GCC, and `/O1` on MSVC and clang-cl. It keeps optimization consistent and avoids the OptimizationLevel differs error.
- Linker `/OPT:REF` and `/OPT:ICF`, which drop unused code and fold identical functions.

These apply to non-debug builds only. The compile flag is guarded so Debug builds stay debuggable.

## The hook generator

The most involved part is the hook generator. `emit/hooks.py` emits one C++ function per hookable game method. That function:

1. Runs the registered `before` callbacks, pushing `self` and the arguments to Lua.
2. Applies an argument override or a skip, if the callback returned one.
3. Calls the original method, unless it was skipped.
4. Runs the registered `after` callbacks, and applies a return override if one was given.

`emit/cxx_templates.py` emits the shared hook runtime that stores callbacks, sorts them by priority, and calls them.
Invalid hook overrides are type-checked via pcall. Wrong types log an error and are ignored.

Binding and hook decode share `convert/marshalling.py` `emit_stack_check`, which emits `luax::check<>` calls for all marshallable types.
The Luau type emitter (`emit/luau_types/`) emits the `geode` namespace stub, including `HookHandle` and `HookCallbackTable`.

A method is hookable only when its address is known on the target platform and its arguments and return can be marshalled.
Static methods, constructors, and destructors are skipped.

## Inputs that trigger a rebuild

The custom command depends on the Python sources, the Bromas, the extra stubs in `tools/luau_codegen/extra_bindings/`, and the Geode UI headers.
Rebuild also triggers on extra classes from `Geode/ui/*.hpp` (via `parse/geode_sdk.py`)
and utility free-function headers (`utils/general.hpp`, `utils/string.hpp`, `utils/random.hpp`, `utils/cocos.hpp`).
A change to any of these reruns codegen.

## Geode SDK scan scope

`parse/geode_sdk.py` discovers extra bindings straight from the installed Geode SDK headers:

- **UI classes and enums**: from `Geode/ui/*.hpp` headers included by `UI.hpp`. Only these are bindable, as other headers aren't included in codegen.
- **Free functions**: from the headers and namespaces listed in `_FUNCTION_SOURCES`, mainly `geode::utils` (including its subnamespaces), `geode::cocos`, `geode::createQuickPopup`, and Geode UI functions. The free-function binding file has a fixed `#include` list, so new sources must be manually added there.

Only `GEODE_DLL` declarations are processed. Functions must be marshallable (no out/non-const ref args), or they're dropped.
Only the first free function per name and arity is kept. The rest are logged as `free-function-ambiguous-arity:<n>`.
Only headers included by `Geode/ui/UI.hpp` are scanned for UI classes. There is no extra step that removes headers after filtering.

Some headers are purposely ignored:

- `web.hpp`
- `utils/Task.hpp`
- `utils/async.hpp`
- `utils/file.hpp`

These are excluded because async, HTTP, and file I/O are handled separately.
`web.hpp` is exposed through the handwritten `GeodeWebBinding.cpp` binding instead of the free-function scanner.

## Handwritten type-stub fields

Some bindings are written by hand in `src/lua/bindings/`, not generated. Their type
signatures still need to be in `types/geode.d.luau`. There are two ways to add them:

- `tools/luau_codegen/extra_bindings/*.dluau`: appended at the end of the stub. Use it for new globals like `task` or `imgui`, and for support types the `geode` namespace references, such as `WebNamespace`, `ModNamespace`, `JsonNamespace`, `FsNamespace`, `HookHandle`, and `HookCallbackTable`. Order does not matter for `export type` aliases, so they resolve even after `declare geode`.
- `tools/luau_codegen/emit/luau_types/manual_fields.py`: injects fields into a namespace that codegen already emits, such as `geode.cocos`. A trailing `.dluau` cannot reopen that table, so fields that live under a generated namespace go here.

Some namespaces have functions implemented by hand in C++ (`geode.cocos`,
`geode.utils.base64`, `geode.utils.permission`, `geode.ColorProvider`,
`geode.VersionInfo`, and `geode.Keybind`). For these, the runtime registration in
`src/lua/bindings/geode/*.cpp` and the `manual_fields.py` declarations are two
copies of the same surface, so they must stay in sync. When you add, remove, or
rename a registered function, update `manual_fields.py` to match.
`tests/luau_codegen/test_manual_fields_sync.py` checks that the two match and fails
CI on drift.

## Metadata outputs

After generation, `build/luauapi-gen/` holds helper files for debugging codegen:

- `schema.json` describes the binding plan.
- `report.md` summarizes what was generated.
- `parity.json` records overload and parity checks.
- `audit.md` groups callback, delegate, container, and related skips by category. Regenerated on every codegen run, standalone via `--audit-report-out`.

`emit/metadata.py` writes `schema.json` and `report.md`. `emit/audit.py` writes `audit.md`.
The CLI only orchestrates paths and I/O.

## Free-function platform policy

Free functions are filtered per platform, then intersected just like class methods.
Only functions supported on all key platforms are exposed in Lua, making the bindings platform-independent.

### When to add an override

Add a `FreeFnOverride` if Broma lists overloads not present on a platform and emitting them would break or mislead.
Leave `win` unrestricted unless shown otherwise.

Each override sets `namespace`, `name`, and optionally:

- `allowed_arities_by_platform`: platform ids to allowed argument counts. Platforms not listed get all arities.
- `excluded_platforms`: platform ids where the function is not bound at all (e.g. missing implementation).

### Current overrides

| Function | Restriction | Details |
| --- | --- | --- |
| `geode::utils::game::restart` | arity limit | `android`, `android32`, `android64`, `ios`, `mac`, `imac`, `m1`: 1 arg only |
| `geode::utils::getEnvironmentVariable` | excluded | `ios` only, Geode iOS loader has no implementation |

`win` is unlisted for `restart`, so both overloads remain in the win platform plan.
The final intersected plan still drops the two-argument overload because other intersection platforms only allow one argument.

### Platform groups

If multiple platforms use the same ABI rule, list each platform key in `allowed_arities_by_platform` (no group alias yet).
For `restart`, all mobile and Apple targets allow one argument.

- Android family: `android`, `android32`, `android64`
- Apple family: `ios`, `mac`, `imac`, `m1`

Only add a new platform key after confirming its SDK matches. Do not copy from other families without checking.

`group_supported_free_functions()` filters and deduplicates free functions per platform.
Intersection then removes any not present on all main platforms, using `intersection-missing-platform:<platforms>`.
Free functions that reference skipped or inaccessible object classes are dropped with `not-callable-type:<platform>:<Class>`.

## Field and encrypted-value policy

Class fields are filtered in `policy/fields.py` by `bindable_field()`.

A field is skipped when:

- Its type cannot be marshalled
- It's an array or reference
- It's a function pointer or string pointer
- It's listed in `INACCESSIBLE_FIELDS` in `tools/luau_codegen/policy/fields.py`.

Skipped fields still appear as `-- skipped <name>: <reason>` comments in the type stub.

Obfuscated or encrypted value fields, such as Geometry Dash's `SeedValue`, `SeedValueRR`, `SeedValueSFd`,
and other similar types, are **not bound by policy**.
These fields do not have a type that can be safely or correctly converted for Lua,
so they are skipped by the generic `unsupported-arg` or `unsupported-return` rule.
This is intentional. Encrypted or obfuscated fields are unsafe and useless in Lua.
Keep them skipped unless a safe, **read-only** proxy is added. A plain field binding is never the answer.

`gd::map` and `gd::set` field setters use `luax::assignMap` / `luax::assignSet` (clear plus per-entry insert) instead of whole-container `operator=`,
because Geode gnustl on Android does not implement `_Rb_tree::_M_move_assign`.

`gd::vector` primitive field setters (`gd::vector<int>`, `gd::vector<bool>`,
and pointer fields such as `gd::vector<float>*`) use `luax::assignPrimitiveVector` (clear plus `push_back`) instead of whole-container `operator=`.
This is required for `vector<bool>` on Android, where gnustl's `vector<bool>::swap` does not accept `_Bit_iterator` arguments.

`std::pair` inside containers is bound. Pair bodies use `{ first, second }`.
Maps with a pair key use an entry list with `first`, `second`, and `value`.
Maps with a nested `gd::vector<T*>` value bind only for audited shapes (scalar or pair keys, object or opaque elements).
See [Nested containers](nested-containers.md) and [Pair containers](pair-containers.md).

`cocos2d::ccCArray*` fields bind only when listed in `model/cc_c_array.py` (dispatcher handler add/remove queues).
They are read-only `{ Handler? }` views backed by `ReadOnlyCCArrayView.hpp`. Raw `cocos2d::ccArray*` internals stay skipped.
See [ccCArray fields](cc-c-array.md).

## Overload resolution

Broma can declare several methods with the same name.
The generator groups them by name and by input arity (`group_supported()` in `policy/filtering.py`).
When two overloads share a name **and** the same input arity, only one can be bound, because Lua dispatches on arity, not on argument types.

When two overloads share a name and input arity, the generator keeps one only if `PREFERRED_OVERLOADS` lists its normalized arg signature.
Otherwise every colliding overload is skipped with `ambiguous-overload-arity:<arity>` and the method is not bound.
These skips appear in `report.md` and `schema.json` under `ambiguousOverloads`.

Ambiguous overloads always cause codegen to exit with code 6 when building the emit plan.
Add or fix a `PREFERRED_OVERLOADS` entry for that `(class, method)`.

## Denylist maintenance

`model/denylist.py` is hand-maintained. It has four structures:

- `INACCESSIBLE_METHODS`: `(class, method)` pairs that must never bind (engine internals, editor-only delegates, unsafe helpers).
- `INACCESSIBLE_CLASSES`: whole classes treated as opaque.
- `PREFERRED_OVERLOADS`: for a `(class, method)` where overloads collide on input arity, the normalized arg signatures to keep. Others skip as `overload-superseded:<arity>`. Required when multiple same-arity overloads must bind.
- `BINDABLE_CONSTRUCTORS`: narrow opt-in list of constructor signatures that get a synthetic `new(...)` factory. See Constructor binding below.

Entries are conservative. Removing one can re-expose a method that breaks marshalling.
Do not expand `PREFERRED_OVERLOADS` without a real overload clash.

`tests/luau_codegen/test_denylist.py` catches rot.
It loads the parsed Broma model and fails if any denylist entry or `PREFERRED_OVERLOADS` signature no longer resolves after a GD or SDK bump.
It auto-skips when bindings are not checked out. Set `LUAUAPI_BINDINGS_DIR` to run it outside a build tree.

Constructors are blocked here too. See below.

## Constructor binding

Constructors and destructors are rejected in `policy/filtering.py` (`supported()` returns `constructor` or `destructor`).
Raw constructors are usually wrong. `new` on a `CCObject` without `autorelease()` leaks.
Nearly every class already has a static `create()` factory.

`BINDABLE_CONSTRUCTORS` in `model/denylist.py` is a narrow opt-in when `create()` is filtered out and Lua has no other way to construct the type.
Each listed constructor signature becomes a synthetic static `new(...)` factory.
The generated `new` calls `autorelease()` and is available as `<Class>Factory.new` in the stub.

Keep the list small. Vet each entry. Plain `new T(args)` must produce a valid object.
Classes that need a constructor plus a separate `init()` do not qualify. Destructors stay blocked.

For how one type stub stays safe on every platform, see [Platform parity](platform-parity.md).

## CLI exit codes

| Code | Meaning |
| --- | --- |
| 2 | Bad arguments, missing required directories, or Python version below 3.11 |
| 3 | No Broma classes found after parsing |
| 4 | I/O error reading inputs or writing outputs |
| 5 | Unexpected exception during emit |
| 6 | Ambiguous overloads remain after building the emit plan |

## Regenerating delegates

The main codegen stamp (`luauapi_codegen`) collects delegate specs and emits trampolines before bindings.
To run only the delegate step:

```bash
PYTHONPATH=tools python -m luau_codegen --emit-delegates --bindings <bindings-dir> --out <gen-dir>
```

Outputs include `build/luauapi-gen/delegate_specs.py` and `build/luauapi-gen/src/lua/bindings/framework/LuaDelegates.gen.hpp` / `.cpp`.
See [Reference: delegates](../lua/reference/delegates.md) for how scripts use delegate tables.

## Tests

The Python code generator has a unit test suite under `tests/luau_codegen/`, run through CTest as `luauapi_codegen_tests`.
See [Testing](testing.md).

## Source

- `CMakeLists.txt`
- `tools/luau_codegen/cli/main.py`
- `tools/luau_codegen/cli/io.py`
- `tools/luau_codegen/parse/collect.py`
- `tools/luau_codegen/policy/free_functions.py`
- `tools/luau_codegen/policy/filtering.py`
- `tools/luau_codegen/model/denylist.py`
- `tools/luau_codegen/emit/metadata.py`
- `tools/luau_codegen/emit/audit.py`
- `tools/luau_codegen/emit/hooks.py`
- `tools/luau_codegen/emit/cxx_templates.py`
- `tools/luau_codegen/emit/luau_types/`
- `tools/luau_codegen/convert/marshalling.py`
- `tools/luau_codegen/emit/bindings/`
