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

The platform comes from the build target (Windows maps to `win`, an Android 64-bit build to `android64`).
The GD version is read from the `"win"` entry in `mod.json`.
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

- UI classes and enums from `Geode/ui/*.hpp` headers included by `UI.hpp`.
- Free functions from headers and namespaces in `model/free_fn_sources.py` (`FREE_FUNCTION_SOURCES`),
  mainly `geode::utils`, `geode::cocos`, `geode::createQuickPopup`, and Geode UI functions.
  The scanner and the generated binding file include list both derive from that one manifest,
  kept in sync by `test_binding_guards.py`.

Only `GEODE_DLL` declarations are processed, and functions must be marshallable.
Only the first free function per name and arity is kept.
`web.hpp`, `utils/Task.hpp`, `utils/async.hpp`, and `utils/file.hpp` are purposely ignored,
since async, HTTP, and file I/O are handled separately.

## Handwritten type-stub fields

Some bindings are written by hand in `src/bindings/` and `src/framework/`.
Their type signatures still need to be in `types/geode.d.luau`.

Two ways to add them:

- `tools/luau_codegen/extra_bindings/*.dluau`: appended at the end of the stub.
  Use it for new globals like `task` or `imgui`, and for support types the `geode` namespace references.
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
- `test_binding_guards.py` enforces that split

## Metadata outputs

After generation, `build/luauapi-gen/` holds:

- `schema.json`: the binding plan
- `report.md`: a summary
- `parity.json`: overload and parity checks
- `audit.md`: callback, delegate, container, and related skips by category

`emit/metadata.py` writes the schema and report
`emit/audit.py` writes the audit, can also be run standalone via `--audit-report-out`

## Overload resolution

Broma can declare several methods with the same name.
The generator groups them by name and input arity (`group_supported()` in `policy/filtering.py`).
When two overloads share a name and input arity, only one can bind, because Lua dispatches on arity.
The generator keeps one only if `PREFERRED_OVERLOADS` lists its normalized arg signature,
otherwise every colliding overload is skipped with `ambiguous-overload-arity:<arity>`.
Ambiguous overloads cause codegen to exit with code 6 when building the emit plan.

## Field and container policy

Class fields are filtered in `policy/fields.py` by `bindable_field()`.

A field is skipped when any of the following apply:

- Its type cannot be marshalled
- It is an array or reference
- It is a function or string pointer
- It is listed in `INACCESSIBLE_FIELDS`
- It is an obfuscated or encrypted value field (such as `SeedValue`, which are not bound by policy)

Skipped fields appear as `-- skipped <name>: <reason>` comments in the stub.

`gd::map` and `gd::set` field setters use `assignMap` / `assignSet` (clear plus per-entry insert)
instead of whole-container `operator=`, because Geode gnustl on Android lacks `_Rb_tree::_M_move_assign`.
`gd::vector` primitive field setters use `assignPrimitiveVector`. `std::pair` inside containers is bound.
See [Pair containers](pair-containers.md), [Nested containers](nested-containers.md), and [ccCArray fields](cc-c-array.md).

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

## Regenerating delegates

The main stamp collects delegate specs and emits trampolines before bindings.

To run only the delegate step:

```bash
PYTHONPATH=tools python -m luau_codegen --emit-delegates --bindings <bindings-dir> --out <gen-dir>
```

See [delegates reference](../../reference/lua/delegates.md) for how scripts use delegate tables.

## Related

- [Platform parity](platform-parity.md)
- [Pair containers](pair-containers.md)
- [Nested containers](nested-containers.md)
- [ccCArray fields](cc-c-array.md)
- [Testing](../testing.md)

## Source

- `CMakeLists.txt`
- `tools/luau_codegen/cli/main.py`
- `tools/luau_codegen/parse/collect.py`
- `tools/luau_codegen/policy/filtering.py`
- `tools/luau_codegen/model/denylist.py`
- `tools/luau_codegen/emit/hooks.py`
- `tools/luau_codegen/emit/cxx_templates.py`
- `tools/luau_codegen/emit/luau_types/`
- `tools/luau_codegen/convert/marshalling.py`
- `tools/luau_codegen/emit/bindings/`
