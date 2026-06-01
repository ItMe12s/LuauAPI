# Codegen

## Summary

The binding code generator turns Geode binding files into C++ bindings and Luau type stubs. It is a Python tool in `tools/luau_codegen/`.
Overview of the binding generator, not a full reference for every internal module.

## What it produces

The generator reads Broma binding files for one Geometry Dash version and one platform. It writes:

- C++ binding sources into `build/luauapi-gen/src`. These expose game classes to Lua and implement the hook functions.
- A single Luau type stub, `types/geode.d.luau`, holding the bound classes, the per-namespace factories, the enum aliases, and the `geode` namespace root. It gives editors autocomplete and type checks.
- Metadata files, including a schema, a report, and a parity file.

The generated C++ is compiled into the main library.
Because it is machine written, you do not edit it, and the build compiles it with warnings disabled.

## How the build calls it

`CMakeLists.txt` runs `python -m luau_codegen` (the package in `tools/luau_codegen/`,
with `tools/` on `PYTHONPATH`) as the `luauapi_codegen` target, and the main library depends on this target.
The build calls it three times. Two calls list expected outputs (`--list-outputs` and `--list-type-outputs`) so CMake knows the byproducts.
The third call performs generation and writes a stamp file.

The platform is chosen from the build target. Windows maps to `win`, for example, and an Android 64-bit build maps to `android64`.
The Geometry Dash version is read from the `"win"` entry in `mod.json`.

## The hook generator

The most involved part is the hook generator. `emit/hooks.py` emits one C++ function per hookable game method. That function:

1. Runs the registered `before` callbacks, pushing `self` and the arguments to Lua.
2. Applies an argument override or a skip, if the callback returned one.
3. Calls the original method, unless it was skipped.
4. Runs the registered `after` callbacks, and applies a return override if one was given.

`emit/cxx_templates.py` emits the shared hook runtime that stores callbacks, sorts them by priority, and calls them.
Invalid hook overrides are type-checked via pcall, wrong types log an error and are ignored.

Binding and hook decode share `convert/marshalling.py` `emit_stack_check`, which emits `luax::check<>` calls for all marshallable types.
The Luau type emitter (`emit/luau_types/`) emits the `geode` namespace stub, including `HookHandle` and `HookCallbackTable`.

A method is hookable only when its address is known on the target platform and its arguments and return can be marshalled.
Static methods, constructors, and destructors are skipped.

## Inputs that trigger a rebuild

The custom command depends on the Python sources, the Bromas, the extra stubs in `tools/luau_codegen/extra_bindings/`, and the Geode UI headers.
Rebuild input also includes classes discovered by `tools/luau_codegen/parse/geode_sdk.py` from `Geode/ui/*.hpp`.
A change to any of these reruns codegen.

## Metadata outputs

After generation, `build/luauapi-gen/` holds helper files for debugging codegen:

- `schema.json` describes the binding plan.
- `report.md` summarizes what was generated.
- `parity.json` records overload and parity checks.

`emit/metadata.py` writes `schema.json` and `report.md`. The CLI only orchestrates paths and I/O.

## Free-function platform policy

Namespace free functions (e.g. `geode::utils::game::restart`) are filtered in `policy/free_functions.py` before binding and type emission.

### When to add an override

Add a `FreeFnOverride` if Broma lists overloads not present on a platform and emitting them would break or mislead.
Leave `win` unrestricted unless shown otherwise.

Each override sets `namespace`, `name`, and `allowed_arities_by_platform` (platform ids to allowed argument counts).
Platforms not listed get all arities.

### Current overrides

| Function | Platforms with restriction | Allowed arities |
| --- | --- | --- |
| `geode::utils::game::restart` | `android`, `android32`, `android64`, `ios`, `mac`, `imac`, `m1` | 1 only |

`win` is unlisted, both overloads remain until two-argument support is checked on Windows.

### Platform groups

If multiple platforms use the same ABI rule, list each platform key in `allowed_arities_by_platform` (no group alias yet).
For `restart`, all mobile and Apple targets allow one argument.

- Android family: `android`, `android32`, `android64`
- Apple family: `ios`, `mac`, `imac`, `m1`

Only add a new platform key after confirming its SDK matches. Don’t copy from other families without checking.

`free_function_supported()` skips functions with un-marshallable types.

## Tests

The Python code generator has a unit test suite under `tests/luau_codegen/`, run through CTest as `luauapi_codegen_tests`.
See [Testing](testing.md).

## Source

- `CMakeLists.txt`
- `tools/luau_codegen/cli/main.py`
- `tools/luau_codegen/cli/io.py`
- `tools/luau_codegen/parse/collect.py`
- `tools/luau_codegen/policy/free_functions.py`
- `tools/luau_codegen/emit/metadata.py`
- `tools/luau_codegen/emit/hooks.py`
- `tools/luau_codegen/emit/cxx_templates.py`
- `tools/luau_codegen/emit/luau_types/`
- `tools/luau_codegen/convert/marshalling.py`
- `tools/luau_codegen/emit/bindings/`
