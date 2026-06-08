# Bindings framework

## Summary

The bindings framework exposes C++ types to Lua.
It lives in `src/lua/bindings/framework/`, with the registry in `src/lua/bindings/Binding.hpp`.
This page explains how to register a binding and how the pieces fit together.

## The binding registry

A binding is a function that takes a `lua_State*` and returns a `geode::Result<void>`.
Register it with `LUAX_BINDING`.
Each binding runs once when the runtime is built.

`LUAX_BINDING` registers the function with a default priority of `10`.
Use `LUAX_BINDING_PRIORITY` to set a different priority.
`applyAllBindings` runs every binding in priority order at startup.
The task library follows this pattern in `src/lua/bindings/task/TaskBinding.cpp`.

## Handwritten bindings

Most game types come from codegen. A few libraries are handwritten in C++:

- `src/lua/bindings/geode/GeodeFsBinding.cpp` exposes `geode.fs`.
- `src/lua/bindings/geode/GeodeJsonBinding.cpp` exposes `geode.json`.
- `src/lua/bindings/geode/GeodeModBinding.cpp` exposes `geode.Mod`.
- `src/lua/bindings/geode/GeodeWebBinding.cpp` registers `geode.utils.web`.
  Web logic lives in `GeodeWebRequest.cpp`, `GeodeWebOptions.cpp`, `GeodeWebResponse.cpp`, `GeodeWebMultipart.cpp`, and `GeodeWebListeners.cpp`.
  Shared caps and helpers live in `WebCaps.hpp` and `GeodeWebInternal.hpp`.
- `src/lua/bindings/geode/GeodeBase64Binding.cpp` exposes `geode.utils.base64`.
- `src/lua/bindings/geode/GeodePermissionBinding.cpp` exposes `geode.utils.permission`.
- `src/lua/bindings/geode/GeodeCocosBinding.cpp` exposes the handwritten `geode.cocos` helpers.
- `src/lua/bindings/geode/GeodeColorProviderBinding.cpp` exposes `geode.ColorProvider`.
- `src/lua/bindings/geode/GeodeKeybindBinding.cpp` exposes `geode.Keybind`.
- `src/lua/bindings/geode/GeodeVersionBinding.cpp` exposes `geode.VersionInfo`.
- `src/lua/bindings/task/TaskBinding.cpp` exposes `task` and `time`.
- `src/lua/bindings/imgui/ImGuiBinding.cpp` exposes `imgui`.

Generated free functions in `bindings_free_functions.cpp` cover the rest of `geode.cocos`.
Luau types for these bindings come from `tools/luau_codegen/emit/luau_types/` or `tools/luau_codegen/extra_bindings/`.

## Usertypes

`Usertype<T>` exposes a C++ type as Lua userdata with a metatable. `T` must derive from `cocos2d::CCObject`.

The main calls are:

- `registerType(L, name, baseTags)` registers the type once, with the base type tags for inheritance.
- `method(L, name, fn)` adds a method.
- `field(L, name, getter, setter)` adds a property.
- `check(L, idx, method)` reads and validates the userdata, or raises an error.
- `tryCheck(L, idx)` reads the userdata, or returns null.
- `pushOwned(L, obj)` pushes the object as Lua owned.
- `pushBorrowed(L, obj)` pushes the object as a weak borrow.

The metatable holds a methods table, a fields table, and `__index` and `__newindex` handlers.
The handlers search methods first, then fields, then the per node field table for `CCNode` types.

## The usertype registry

`UsertypeRegistry` maps each C++ type to a small integer tag and back. Tags are stable for the life of the runtime.
Each type record holds its tag, name, metatable name, and base tags for inheritance checks.

## Ownership

An owned userdata holds a strong pointer and increases the C++ retain count.
A borrowed userdata holds a weak reference and does not retain.
The global retain map in `Ref.hpp` tracks owned objects.
When an owned userdata is collected, the retain is released, and on shutdown all Lua retains are cleared.

## The stack layer

`Stack.hpp` provides overloaded `push` and `check` helpers for primitives and strings.
It also provides helpers for reading numeric and boolean table fields,
and helpers to write integers as strings, which avoids float precision loss for large values.

## Value types

`Types.hpp` provides `check<T>` specializations and `push(T)` overloads for cocos value types:
points, sizes, rects, colors, and button configs.

Generated binding and hook code uses these helpers to decode and push table-shaped values.
The generated hook runtime applies overrides through a pcall trampoline so decode errors are safe.

## Container tables

`ContainerTables.hpp` marshals `gd::vector`, `gd::map`, `gd::set`, and related types to Luau tables.
Primitives and objects use array or dictionary shapes. `std::pair` uses `{ first, second }`.
Maps with a pair key use an entry list (see [Pair containers](pair-containers.md)).
Field setters call `assignMap`, `assignSet`, or `assignPrimitiveVector` instead of assigning whole containers.

## Lua references

`LuaRef` wraps a Lua registry reference with RAII.
It records the runtime generation and the resources root, and it becomes invalid after a runtime restart.
The task scheduler and the hook runtime store callbacks as `LuaRef`.

## Callback bridge

`LuaCallback` centralizes the pattern for invoking a stored Luau function from C++:
push the registry ref, apply `ResourcesRootScope`, call `Runtime::protectedCall`, and restore the stack top.

Generated bindings use `LuaCallback` when a method argument is a:

- `std::function`
- `Function`
- `MiniFunction`
- `Callback` (including non-void returns).

For cocos2d `SEL_*` pairs `(CCObject* target, SEL_... selector)`, codegen collapses the pair into one Luau function
and creates the matching handler trampoline (`LuaMenuHandler`, `LuaScheduleHandler`, `LuaCallFunc*Handler`).
For delegate pointer arguments, codegen accepts a Luau table and creates a `Lua*` delegate trampoline (`LuaDelegate` + generated subclasses).

Ignored callback failures are logged and keep their existing lifetime semantics:

- Selector
- Menu
- Delegate
- Setting
- Web
- Permission

See [Reference: callbacks](../lua/reference/callbacks.md) and [Reference: delegates](../lua/reference/delegates.md) for script-facing usage.

## Per node fields

`Fields` gives each `CCNode` a persistent Lua table and caches it in a map keyed by the node.
The table is evicted when the node is released. This backs `geode.fields` and the `m_fields` property.

## Adding a new bound type

1. Register the type with `registerType`, passing the base tags.
2. Add methods with `method` and properties with `field`.
3. Push instances with `pushOwned` or `pushBorrowed`.
4. Read arguments with `check` or `tryCheck`.

In practice most game types are generated. See [Codegen](codegen.md).

## Source

- `src/lua/bindings/Binding.hpp`
- `src/lua/bindings/Binding.cpp`
- `src/lua/bindings/framework/Usertype.hpp`
- `src/lua/bindings/framework/UsertypeRegistry.cpp`
- `src/lua/bindings/framework/Stack.hpp`
- `src/lua/bindings/framework/LuaRef.hpp`
- `src/lua/bindings/framework/LuaCallback.hpp`
- `src/lua/bindings/framework/LuaMenuHandler.hpp`
- `src/lua/bindings/framework/Ref.hpp`
- `src/lua/bindings/framework/Types.hpp`
- `src/lua/bindings/framework/ContainerTables.hpp`
- `src/lua/bindings/framework/Fields.cpp`
