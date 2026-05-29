# Codegen

## Summary

The code generator turns Geode binding files into C++ bindings and Luau type stubs. It is a Python tool in `tools/luau_codegen/`. This page is an overview rather than a full reference for every internal module.

## What it produces

The generator reads Broma binding files for one Geometry Dash version and one platform. It writes:

- C++ binding sources into `build/luauapi-gen/src`. These expose game classes to Lua and implement the hook functions.
- Luau type stubs into `types/`. These give editors autocomplete and type checks.
- Metadata files, including a schema, a report, and a parity file.

The generated C++ is compiled into the main library. Because it is machine written, you do not edit it, and the build compiles it with warnings disabled.

## How the build calls it

`CMakeLists.txt` runs `tools/luau_codegen/codegen.py` as the `luauapi_codegen` target, and the main library depends on this target. The build calls the generator to list its expected outputs, then once more to generate them and write a stamp.

The platform is chosen from the build target. Windows maps to `win`, for example, and an Android 64 bit build maps to `android64`. The Geometry Dash version is read from `mod.json`.

## The hook generator

The most involved part is the hook generator. `hooks.py` emits one C++ function per hookable game method. That function:

1. Runs the registered `before` callbacks, pushing `self` and the arguments to Lua.
2. Applies an argument override or a skip, if the callback returned one.
3. Calls the original method, unless it was skipped.
4. Runs the registered `after` callbacks, and applies a return override if one was given.

`cxx_templates.py` emits the shared hook runtime that stores callbacks, sorts them by priority, and calls them. `emit_luau_types.py` emits the `geode` namespace stub, including `HookHandle` and `HookCallbackTable`.

A method is hookable only when its address is known on the target platform and its arguments and return can be marshalled. Static methods, constructors, and destructors are skipped.

## Inputs that trigger a rebuild

The custom command depends on the Python sources, the Broma files, the extra binding stubs in `tools/luau_codegen/extra_bindings/`, and the Geode UI headers. A change to any of these reruns codegen.

## Tests

The hook generation logic has a Python test, `tests/luau_codegen_hook_tests.py`, run through CTest. See [Testing](testing.md).

## Source

- `CMakeLists.txt`
- `tools/luau_codegen/codegen.py`
- `tools/luau_codegen/hooks.py`
- `tools/luau_codegen/cxx_templates.py`
- `tools/luau_codegen/emit_luau_types.py`
- `tools/luau_codegen/emit_luau_bindings.py`
