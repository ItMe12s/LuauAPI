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
Rebuild also triggers on extra classes from `Geode/ui/*.hpp` (via `parse/geode_sdk.py`)
and utility free-function headers (`utils/general.hpp`, `utils/string.hpp`, `utils/random.hpp`).
A change to any of these reruns codegen.

## Geode SDK scan scope

`parse/geode_sdk.py` discovers extra bindings straight from the installed Geode SDK headers:

- **UI classes and enums**: from `Geode/ui/*.hpp` headers included by `UI.hpp`. Only these are bindable, as other headers aren't included in codegen.
- **Free functions**: from the headers and namespaces listed in `_FUNCTION_SOURCES`, mainly `geode::utils` (including its subnamespaces), `geode::createQuickPopup`, and Geode UI functions. The free-function binding file has a fixed `#include` list, so new sources must be manually added there.

Only `GEODE_DLL` declarations are processed. Functions must be marshallable (no out/non-const ref args), or they're dropped.
Keep only the first free function per name and arity, log others as `free-function-ambiguous-arity:<n>`.
Remove headers that yield nothing after filtering.

Some headers are purposely ignored:

`web.hpp`, `utils/Task.hpp`, `utils/async.hpp`, and `utils/file.hpp` because async, HTTP, and file I/O are handled separately.

## Metadata outputs

After generation, `build/luauapi-gen/` holds helper files for debugging codegen:

- `schema.json` describes the binding plan.
- `report.md` summarizes what was generated.
- `parity.json` records overload and parity checks.
- `audit.md` groups callback, delegate, container, and related skips by category. Regenerated on every codegen run, standalone via `--audit-report-out`.

`emit/metadata.py` writes `schema.json` and `report.md`. `emit/audit.py` writes `audit.md`. The CLI only orchestrates paths and I/O.

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

Only add a new platform key after confirming its SDK matches. Don’t copy from other families without checking.

`group_supported_free_functions()` filters and deduplicates free functions per platform,
intersection removes any not present on all main platforms, using `intersection-missing-platform:<platforms>`.
Free functions that reference skipped or inaccessible object classes are dropped with `not-callable-type:<platform>:<Class>`.

## Field and encrypted-value policy

Class fields are filtered in `policy/fields.py` by `bindable_field()`.

A field is skipped when:

- Its type cannot be marshalled
- It's an array or reference
- It's a function pointer or string pointer
- It's listed in `INACCESSIBLE_FIELDS`.

Skipped fields still appear as `-- skipped <name>: <reason>` comments in the type stub.

Obfuscated or encrypted value fields, such as Geometry Dash's `SeedValue`, `SeedValueRR`, `SeedValueSFd`,
and other similar types, are **not bound by policy**.
These fields do not have a type that can be safely or correctly converted for Lua,
so they are skipped by the generic `unsupported-arg` or `unsupported-return` rule.
This is intentional. Encrypted or obfuscated fields are unsafe and useless in Lua.
Keep them skipped unless a safe, **read-only** proxy is added, a plain field binding is never the answer.

## Overload resolution

Broma can declare several methods with the same name.
The generator groups them by name and by input arity (`group_supported()` in `policy/filtering.py`).
When two overloads share a name **and** the same input arity, only one can be bound, because Lua dispatches on arity, not on argument types.

The rule is **first-declared wins**.
Any later overload with the same name and number of arguments is skipped with the reason `ambiguous-overload-arity:<arity>`.
These skipped overloads are listed in `report.md` and also in `schema.json` under `ambiguousOverloads`.

By default, the build fails with exit code 6 if ambiguous overloads are found.
This is controlled by the `--fail-on-ambiguous-overload` CLI flag.
This ensures you resolve overload ambiguities instead of relying on declaration order.
Resolve ambiguities by moving the preferred overload to the top or excluding others.

## Denylist maintenance

`model/denylist.py` is hand-maintained. It has three structures:

- `INACCESSIBLE_METHODS`: `(class, method)` pairs that must never bind (engine internals, editor-only delegates, unsafe helpers).
- `INACCESSIBLE_CLASSES`: whole classes treated as opaque.
- `PREFERRED_OVERLOADS`: for a `(class, method)` where overloads collide on input arity, the normalized arg signatures to keep. Others skip as `overload-superseded:<arity>`. Use this instead of **first-declared wins** when you need the `create` or script-friendly overload.

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
