# Bindings framework

## Summary

The bindings framework exposes C++ types to Lua.
It lives in `src/framework/`, with the registry in `src/framework/Binding.hpp`.
This page explains how to register a binding and how the pieces fit together.

## The binding registry

A binding is a function that takes a `lua_State*` and returns a `geode::Result<void>`.
Register it with `LUAX_BINDING`, which uses a default priority of `10`.
Use `LUAX_BINDING_PRIORITY` to set a different priority.
`applyAllBindings` runs every binding in priority order at startup.
Each binding runs once when the runtime is built.

## Handwritten bindings

Most game types come from codegen.

A few libraries are handwritten in C++ under `src/bindings/geode/` and `src/framework/`:

| File | Lua Module / Description |
| --- | --- |
| `GeodeFsBinding.cpp` | `geode.fs` |
| `GeodeJsonBinding.cpp` | `geode.json` |
| `GeodeModBinding.cpp` | `geode.Mod` |
| `GeodeBase64Binding.cpp` | `geode.utils.base64` |
| `GeodePermissionBinding.cpp` | `geode.utils.permission` |
| `GeodeCocosBinding.cpp` | Handwritten `geode.cocos` helpers |
| `GeodeColorProviderBinding.cpp` | `geode.ColorProvider` |
| `GeodeKeybindBinding.cpp` | `geode.Keybind` |
| `GeodeVersionBinding.cpp` | `geode.VersionInfo` |
| `task/TaskBinding.cpp` | `task` and `time` |
| `imgui/ImGuiBinding.cpp` | `imgui` |
| `render3d/TransformBinding.cpp` | `gd3d.Transform` |
| `render3d/GltfBinding.cpp` | `gd3d.gltf` and mesh handles |
| `render3d/MaterialBinding.cpp` | `gd3d.Material` |
| `render3d/ViewportFrameBinding.cpp` | `gd3d.ViewportFrame` |

The web binding is split across several translation units:

| File | Role |
| --- | --- |
| `GeodeWebBinding.cpp` | Entry registration and userdata metatables |
| `GeodeWebApply.cpp` | Shared `apply*` helpers for request options and body caps |
| `GeodeWebOptions.cpp` | Options-table parsing |
| `GeodeWebRequest.cpp` | Fluent `WebRequest` chain methods and `send` |
| `GeodeWebResponse.cpp` | `WebResponse` accessors |
| `GeodeWebMultipart.cpp` | `MultipartForm` builders |
| `GeodeWebListeners.cpp` | Request intercept and response listeners |

Generated free functions in `bindings_free_functions.cpp` cover the rest of `geode.cocos`.
Luau types for these bindings come from `tools/luau_codegen/emit/luau_types/` or `extra_bindings/`.

## Usertypes

`Usertype<T>` exposes a C++ type as Lua userdata with a metatable.

`T` must derive from `cocos2d::CCObject`. The main calls:

- `registerType(L, name, baseTags)` registers the type once, with base type tags for inheritance.
- `method(L, name, fn)` adds a method, `field(L, name, getter, setter)` adds a property.
- `check(L, idx, method)` reads and validates the userdata or raises.
  `tryCheck(L, idx)` returns null on mismatch.
- `pushOwned(L, obj)` pushes the object as Lua owned. `pushBorrowed(L, obj)` pushes a weak borrow.

The metatable holds a methods table, a fields table, and `__index` and `__newindex` handlers.
The handlers search methods first, then fields, then the per-node field table for `CCNode` types.

## The usertype registry

`UsertypeRegistry` maps each C++ type to a small integer tag and back. Tags are stable for the life of the runtime.

Each usertype record contains:

- Tag (unique integer)
- Name
- Metatable name
- Base tags (for inheritance checks)

Tag assignments:

- Reserved tags:
  - `kOpaqueHandleUserdataTag` (1)
  - `kTaskHandleUserdataTag` (2)
  - `kImGuiDrawHandleUserdataTag` (3)
  - `kWsConnectionUserdataTag` (4)
  - `kWsServerUserdataTag` (5)
  - `kWsPeerUserdataTag` (6)
- Dynamic tags:
  - Begin at `kFirstDynamicUsertypeTag` (7)

## Ownership

An owned userdata holds a strong pointer and increases the C++ retain count.
A borrowed userdata holds a weak reference and does not retain. The global retain map in `Ref.hpp` tracks owned objects.
When an owned userdata is collected the retain is released, and on shutdown all Lua retains are cleared.

## Stack, value types, and containers

`Stack.hpp` provides overloaded `push` and `check` helpers for primitives and strings, plus helpers
for reading numeric and boolean table fields and writing integers as strings to avoid float precision loss.

`Types.hpp`:

- `check<T>` and `push(T)` functions for cocos value types:
  - points
  - sizes
  - rects
  - colors
  - button configs

`ContainerTables.hpp`:

- marshals the following C++ containers to Luau tables:
  - `gd::vector`
  - `gd::map`
  - `gd::set`
  - `std::pair`

Maps with a pair key use an entry list. See [Pair containers](../codegen/pair-containers.md).
Field setters call `assignMap`, `assignSet`, or `assignPrimitiveVector` instead of assigning whole containers.

## References and callback bridge

`LuaRef` wraps a Lua registry reference with RAII.
It records the runtime generation and the resources root, and becomes invalid after a runtime restart.
The task scheduler and hook runtime store callbacks as `LuaRef`.

`LuaCallback` centralizes invoking a stored Luau function from C++:

- Pushes the registry reference.
- Applies `ResourcesRootScope`.
- Calls `Runtime::protectedCall`.
- Restores the stack top.

Generated bindings use `LuaCallback` for these argument types:

- `std::function`
- `Function`
- `MiniFunction`
- `Callback`

For `SEL_*` pairs codegen collapses the pair into one Luau function and creates a handler trampoline.
For delegate pointer arguments it accepts a Luau table and creates a `LuaDelegate` trampoline.
Ignored callback failures are logged and keep their lifetime semantics (selector, menu, delegate, setting, web, permission).
See [callbacks](../../reference/lua/callbacks.md) and [delegates](../../reference/lua/delegates.md).

## Per-node fields

`Fields` gives each `CCNode` a persistent Lua table and caches it in a map keyed by the node.
The table is evicted when the node is released. This backs `geode.fields` and the `m_fields` property.

## Adding a new bound type

1. Register the type with `registerType`, passing the base tags.
2. Add methods with `method` and properties with `field`.
3. Push instances with `pushOwned` or `pushBorrowed`.
4. Read arguments with `check` or `tryCheck`.

In practice most game types are generated. See [Codegen](../codegen/codegen.md).

## Related

- [Runtime](runtime.md)
- [Codegen](../codegen/codegen.md)
- [callbacks](../../reference/lua/callbacks.md)
- [delegates](../../reference/lua/delegates.md)

## Source

- `src/framework/Binding.hpp`
- `src/framework/usertype/Usertype.hpp`
- `src/framework/usertype/UsertypeRegistry.cpp`
- `src/framework/stack/Stack.hpp`
- `src/framework/usertype/LuaRef.hpp`
- `src/framework/callback/LuaCallback.hpp`
- `src/framework/usertype/Ref.hpp`
- `src/framework/stack/Types.hpp`
- `src/framework/stack/ContainerTables.hpp`
- `src/framework/usertype/Fields.cpp`
