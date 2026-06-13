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
| `GeodeModBinding.cpp` | `geode.Mod` |
| `GeodeSmallBindings.cpp` | `geode.json`, `geode.utils.base64`, `geode.utils.permission`, `geode.ColorProvider`,`geode.Keybind`, `geode.VersionInfo` (see host-test split below) |
| `GeodeWebBinding.cpp` and siblings under `web/` | `geode.utils.web` |
| `GeodeCocosBinding.cpp` | Handwritten `geode.cocos` helpers |
| `task/TaskBinding.cpp` | `task` and `time` |
| `imgui/ImGuiBinding.cpp` | `imgui` |
| `render3d/Gd3dRegister.cpp` | `gd3d` entry (`registerGd3d`) |
| `render3d/TransformBinding.cpp` | `gd3d.Transform` |
| `render3d/GltfBinding.cpp` | `gd3d.gltf` |
| `render3d/ProceduralMeshBinding.cpp` | `gd3d.mesh` |
| `render3d/TextureBinding.cpp` | `gd3d.texture` |
| `render3d/MaterialBinding.cpp` | `gd3d.Material` |
| `render3d/ViewportFrameBinding.cpp` | `gd3d.ViewportFrame` |
| `render3d/internal/MeshHandleBinding.cpp` | shared `Mesh` userdata metatable |
| `render3d/internal/Marshaling.hpp` | vec3/color parsers |
| `render3d/internal/Handles.hpp` | mesh/material/texture handle types |

The `gd3d` Lua module lives in `src/bindings/render3d/`.
Rendering and asset code lives in `src/render3d/`.

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

The websocket binding lives under `src/bindings/websocket/`:

| File | Role |
| --- | --- |
| `WebSocketBinding.cpp` | Entry registration and userdata metatables |
| `WebSocketConnection.cpp` | `WebSocketConnection` client methods and callbacks |
| `WebSocketServer.cpp` | `WebSocketServer` and `WebSocketPeer` methods, broadcast, and `serve` |
| `WebSocketInternal.hpp` | Shared types, limits, and marshaling helpers |

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
  - `kTransformUserdataTag` (7)
  - `kMeshAssetUserdataTag` (8)
  - `kMaterialUserdataTag` (9)
  - `kTextureUserdataTag` (10)
- Dynamic tags:
  - Begin at `kFirstDynamicUsertypeTag` (11)

Codegen usertypes take the next free dynamic tag at registration time.

## Handle pools

`WeakHandlePool` in `src/framework/lifecycle/WeakHandlePool.hpp` tracks live handles
for subsystems that need runtime shutdown cleanup.

- `track` stores a `shared_ptr` or `weak_ptr`.
- `compactAndCountLive` drops expired weak entries and returns the live count.
- `clearAll` locks each live entry and runs a caller-supplied shutdown callback, then clears the pool.

WebSocket connections and servers use this pool.
On runtime shutdown, `clearWsState` shuts down every live socket and clears the pools.

## Mod sandbox

`ModSandbox.hpp` resolves mod-owned filesystem paths for bindings that read or write user files.

- `resolveSandboxTarget(L, rootIdx, pathIdx, method, requireWritable)` maps a root name
  (`save`, `config`, `persistent`, or `resources`) and a relative path to a canonical path inside that directory.
- `readSandboxTextFile(path)` reads a regular file with the filesystem read cap.

Used by `geode.fs`, `geode.utils.web` (`WebResponse:saveTo`, `MultipartForm:fileFrom`), and `gd3d` asset loaders.

This is separate from `PathSandbox.hpp` in `src/require/`, which serves the public C++ `runFile` path and the `require` loader. Both enforce root containment, but mod sandbox roots come from the current mod directories while require paths stay inside the resources root passed at run time.

## Shutdown hooks

`ShutdownHook.hpp` provides `ensureShutdownHook(registered, clearFn)`.
Subsystems call it once to register a runtime shutdown callback through `Runtime::registerShutdownHook`.
WebSocket registers `clearWsState` this way so open connections and servers close before the Lua state is torn down.

## Host-test binding split

When `LUAUAPI_HOST_TESTS` is defined, several bindings compile a reduced surface so tests avoid Geode-only APIs:

- `GeodeSmallBindings.cpp` registers only `geode.json`.
  Base64, permission, `ColorProvider`, `Keybind`, and `VersionInfo` are omitted.
- `GeodeWebBinding.cpp`, websocket entry registration,
  and several gd3d viewport or GPU paths are wrapped in `#if !defined(LUAUAPI_HOST_TESTS)`.
- Host tests use stubs under `tests/host/` for web async behavior.

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

## Tagged userdata metatables

Handwritten bindings that expose tagged or untagged userdata share `registerTaggedMetatable` in
`src/framework/stack/TaggedMetatable.hpp`.

The helper registers a named metatable from a `luaL_Reg` method table, sets `__index`,

locks `__metatable`, and optionally adds:

- `__type` for Luau type names
- metatable `__gc` (Geode web userdata uses untagged userdata with this path)
- userdata tag linkage via `lua_setuserdatametatable`
- tag destructors via `lua_setuserdatadtor`

Pass `std::nullopt` for `tag` when userdata is plain `lua_newuserdata` without a reserved tag.
Some handles (for example texture and mesh) also expose `__gc` in the method table in addition to a tag destructor.

`ScheduledHandleBinding` uses the same helper for task and imgui draw handles.

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
- `src/framework/stack/TaggedMetatable.hpp`
- `src/framework/usertype/LuaRef.hpp`
- `src/framework/callback/LuaCallback.hpp`
- `src/framework/usertype/Ref.hpp`
- `src/framework/stack/Types.hpp`
- `src/framework/stack/ContainerTables.hpp`
- `src/framework/usertype/Fields.cpp`
- `src/framework/lifecycle/WeakHandlePool.hpp`
- `src/framework/lifecycle/ShutdownHook.hpp`
- `src/bindings/geode/ModSandbox.hpp`
- `src/require/PathSandbox.hpp`
- `src/framework/stack/UserdataTags.hpp`
