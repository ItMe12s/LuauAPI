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
| `GeodeLoaderBinding.cpp` | `geode.Loader` |
| `GeodeSmallBindings.cpp` | `geode.json`, `geode.utils.base64`, `geode.utils.permission`, `geode.ColorProvider`,`geode.Keybind`, `geode.VersionInfo` (see host-test split below) |
| `GeodeKeyboardBinding.cpp` | `geode.KeyboardModifier`, `geode.KeyboardInputData`, `geode.KeyboardInputEvent` |
| `GeodeWebCore.cpp` and siblings under `web/` | `geode.utils.web` |
| `GeodeCocosBinding.cpp` | Handwritten `geode.cocos` helpers |
| `task/TaskBinding.cpp` | `task` and `time` |
| `imgui/ImGuiCore.cpp` | `imgui` |
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

The web binding is split across three translation units:

| File | Role |
| --- | --- |
| `GeodeWebCore.cpp` | Entry registration, userdata metatables, request options, and multipart builders |
| `GeodeWebApi.cpp` | Fluent `WebRequest` chain methods, `send`, and `WebResponse` accessors |
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
- `pushOwnedDynamic` and `pushBorrowedDynamic` pick the closest registered runtime type.

The metatable holds a methods table, a fields table, and `__index` and `__newindex` handlers.
The handlers search methods first, then fields, then the per-node field table for `CCNode` types.

## The usertype registry

`UsertypeRegistry` maps each bound C++ type to an internal type id. Examples: `CCNode`, `CCLayer`.
Ids stay fixed for the whole runtime. This is not a mod count. One id per usertype class, no matter how many mods load.

The registry is process-lifetime. It is not reset when the runtime is recreated.
Do not expect a clean registry after restart in production. Host tests may call `resetForTests()` under `LUAUAPI_HOST_TESTS` only.

Each record holds:

- Tag (internal type id)
- Name
- Metatable name
- Base tags (for inheritance)
- Runtime matcher

`registerType` takes one direct base tag. Its closure fills `baseClosure` for `hasBase` and method lookup.
More than one direct base is an error.

### Luau tag vs internal type id

Luau userdata tags are 8-bit (`uint8_t`). Only 256 Luau tag slots exist. That is too small for one tag per game class.

Usertypes use two layers:

- Luau tag: all `Usertype<T>` userdata share `kSharedUsertypeTag` (12).
- Internal type id: the real class id is in `UserdataBlock::typeTag` and `UsertypeRegistry`.

`requireLive`, `tryCandidate`, and GC check the shared Luau tag first, then read `typeTag`.

On push, `pushUserdataOwned` and `pushUserdataBorrowed` create userdata with the shared Luau tag,
set `block->typeTag`, and call `lua_setmetatable`. One destructor is registered on the shared Luau tag.

Reserved userdata (handles, websocket, gd3d assets, and so on) still get their own Luau tags
through `registerTaggedMetatable` and `lua_setuserdatametatable`.

Tag assignments:

- Reserved Luau tags:
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
  - `kImGuiFontHandleUserdataTag` (11)
- Shared Luau tag for all usertypes:
  - `kSharedUsertypeTag` (12)
- Internal registry ids for usertypes:
  - Start at `kFirstDynamicUsertypeTag` (13)

Codegen picks the next free internal id at registration. That id goes in `UserdataBlock::typeTag`, not in the Luau userdata tag.

## Handle pools

`WeakHandlePool` in `src/framework/lifecycle/Lifecycle.hpp` tracks live handles
for subsystems that need runtime shutdown cleanup.

- `track` stores a `shared_ptr` or `weak_ptr`.
- `compactAndCountLive` drops expired weak entries and returns the live count.
- `clearAll` locks each live entry and runs a caller-supplied shutdown callback, then clears the pool.

Web tasks/listeners and WebSocket connections/servers use this pool.
Web and keyboard listeners share `GeodeListenerState` in `GeodeListenerState.hpp`.
On runtime shutdown, `clearWebState` clears web callbacks and `clearWsState` shuts down every live socket.

## Mod sandbox

`ModSandbox.hpp` resolves mod-owned filesystem paths for bindings that read or write user files.

- `resolveSandboxTarget(L, rootIdx, pathIdx, method, requireWritable)` maps a root name
  (`save`, `config`, `persistent`, or `resources`) and a relative path to a canonical path inside that directory.
- `readSandboxTextFile(path)` reads a regular file with the filesystem read cap.

Used by `geode.fs`, `geode.utils.web` (`WebResponse:saveTo`, `MultipartForm:fileFrom`), and `gd3d` asset loaders.

This is separate from `PathSandbox.hpp` in `src/require/`,
which serves the public C++ `runFile` path and the `require` loader. Both enforce root containment,
but mod sandbox roots come from the current mod directories while require paths stay inside the resources root passed at run time.

## Shutdown hooks

`Lifecycle.hpp` provides `ensureShutdownHook(registered, clearFn)`.
Subsystems call it once to register a runtime shutdown callback through `Runtime::registerShutdownHook`.
WebSocket registers `clearWsState` this way so open connections and servers close before the Lua state is torn down.

Shutdown hooks run in LIFO order (see [Runtime](runtime.md) Shutdown).
The orphan trampoline registry resets its `registered` flag inside its shutdown hook wrapper so a later runtime can register cleanup again.
`LuaCallback::invoke` returns `false` without calling Lua when `Runtime::isShuttingDown()` is true.

## Host-test binding split

When `LUAUAPI_HOST_TESTS` is defined, several bindings compile a reduced surface so tests avoid Geode-only APIs:

- Bindings wrapped in `#if !defined(LUAUAPI_HOST_TESTS)` omit their `LUAX_BINDING` static registrar.
  Host tests call `resetBindingsForTests()`, register only what they need with `registerBinding()`, then call `applyAllBindings`.
- `GeodeSmallBindings.cpp` exposes `registerGeodeJson` for tests.
  Base64, permission, `ColorProvider`, `Keybind`, and `VersionInfo` registrars are omitted.
- `GeodeWebCore.cpp`, websocket entry registration,
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
Nested map and vector shapes are in [Nested containers](../codegen/nested-containers.md).
Field setters call `assignMap`, `assignSet`, or `assignPrimitiveVector` instead of assigning whole containers.

## Tagged userdata metatables

Handwritten bindings that expose tagged or untagged userdata share `registerTaggedMetatable` in
`src/framework/stack/TaggedMetatable.hpp`.

The helper registers a named metatable from a `luaL_Reg` method table, sets `__index`, locks `__metatable`, and optionally adds:

- `__type` for Luau type names
- metatable `__gc` (Geode web userdata uses untagged userdata with this path)
- userdata tag linkage via `lua_setuserdatametatable`
- tag destructors via `lua_setuserdatadtor`

Pass `std::nullopt` for `tag` when userdata is plain `lua_newuserdata` without a reserved tag.
Some handles (for example texture and mesh) also expose `__gc` in the method table in addition to a tag destructor.

Repeat calls with the same tag still set the tag destructor when one is provided.

## OpaqueHandle

`OpaqueHandle` wraps a borrowed C++ pointer. `__gc` does not free the pointee.
Keep storage alive while Lua uses it. `ReadOnlyOpaqueVectorView` holds a weak owner ref.

`Usertype<T>` uses `kSharedUsertypeTag` instead.
`pushOwned` and `pushBorrowed` push `nil` if the metatable was never registered.

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

- [Architecture](../architecture.md)
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
- `src/framework/lifecycle/Lifecycle.hpp`
- `src/framework/lifecycle/GeodeListenerState.hpp`
- `src/bindings/geode/ModSandbox.hpp`
- `src/require/PathSandbox.hpp`
- `src/framework/stack/UserdataTags.hpp`
