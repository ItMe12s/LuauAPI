# Architecture

## Summary

The big picture for contributors.
This page names the main parts and traces how a script gets from a file to running code.

## The parts

- Public C++ API in `imes::luauapi`. The host-facing surface. See [C++ API reference](../reference/cpp/api-reference.md).
- Native registration bridge. Publishes C++ functions and values into the calling mod's Luau table.
  See [Native C++ registration](../reference/cpp/native-registration.md).
- Runtime. Owns the Lua state, memory, deadlines, and the bytecode cache. See [Runtime](internals/runtime.md).
- Bindings framework. Exposes C++ types to Lua. See [Bindings framework](internals/bindings-framework.md).
- Module system. Implements sandboxed `require`. See [Module system](internals/module-system.md).
- Task scheduler. Drives `task` callbacks on the game tick. See [Task scheduler](internals/task-scheduler.md).
- ImGui draw scheduler. Drives `imgui.onDraw` callbacks each frame. See [ImGui draw scheduler](internals/imgui-draw-scheduler.md).
- WebSocket. Client and local server bindings backed by IXWebSocket. See [websocket](../reference/lua/websocket.md).
- Lunar bindings. Sprite rigging and keyframe animation. See [lunar](../reference/lua/lunar.md).
- 3D rendering. Loads glTF meshes and draws them through `gd3d.ViewportFrame` sprites. See [gd3d](../reference/lua/gd3d.md).
- Codegen. Generates the game bindings and the type stubs. See [Codegen](codegen/codegen.md).
- Diagnostics. Crash sidecar for Luau context at native faults. See [Crash sidecar](internals/crash-sidecar.md).
- Release-hook safety. Rule for code reachable from the global `CCObject::release` hook and the deferred release drain.
  See [Release-hook safety](internals/release-hook-safety.md).

## Repository layout

- `include/LuauAPI.hpp`: the public API entry header.
- `include/NativeRegistration.hpp`: the typed registration templates and Lua-free native call bridge.
- `src/api.cpp`: the public API implementation.
- `src/main.cpp`: the mod entry points that drive the runtime lifecycle.
- `src/core/Config.hpp`: the limits and deadlines. See [Limits and errors](../reference/cpp/limits-and-errors.md).
- `src/core/`: the runtime, memory allocator, and small utilities such as the indexed slot map.
- `src/framework/`: the binding registry, usertypes, stack interop, callbacks, views, scheduling,
  and lifecycle helpers (`lifecycle/` for handle pools and shutdown hooks).
- `src/bindings/geode/`: handwritten `geode.*` bindings (web cluster in `web/`).
- `src/bindings/websocket/`: WebSocket client, server, and peer bindings.
- `src/bindings/imgui/`: ImGui binding and draw scheduler.
- `src/bindings/task/`: task scheduler bindings.
- `src/bindings/lunar/`: handwritten `lunar` rig and animation bindings.
- `src/bindings/render3d/`: handwritten `gd3d` bindings.
- `src/render3d/`: glTF loading, GPU rendering, and the `CCViewportFrame` sprite.
- `src/require/`: the requirer and the path rules.
- `src/diagnostics/`: the crash sidecar boundary recorder. See [Crash sidecar](internals/crash-sidecar.md).
- `build/luauapi-gen/src/`: generated C++ bindings from codegen.
- `tools/luau_codegen/`: the Python code generator.
- `mod/luauapi/`: LuauAPI bootstrap, executor, and other scripts. See [Installation](../getting-started/installation.md).
- `mod/demo/`: built-in demo scripts for developer mode and the executor.
- `tests/`: the host tests.

## Lifecycle

The runtime follows the mod and game lifecycle, wired in `src/main.cpp`.
The main thread id is recorded in `$on_mod(Loaded)`, the runtime is created there,
and the bootstrap script runs before the handler returns. Shutdown runs on game exit.

## How a script runs

The host calls `runFile` on the main thread.
The runtime resolves the path, compiles or loads cached bytecode, and runs it under a deadline.
Errors are caught and returned to the host.
See [Runtime](internals/runtime.md) for the full pipeline.

## How native registration runs

The caller-inline templates infer the registering mod and validate the C++ type shape.
They pass copied function pointer bytes, an invoker, and scalar data through a Lua-free bridge to `src/api.cpp`.
The runtime publishes the value through a protected Lua call and raw table access.
It builds missing tables away from `_G`, then attaches the completed branch in one write.

A registered closure owns the copied bridge data in the Lua state.
It invokes native code through the coroutine state that called it and records a native-function diagnostic boundary.
Closing the Lua state releases the closure storage without calling the registering mod.

## Request-driven flows

- **Hooks.** A script registers with `geode.hook`. Generated hook code runs before callbacks, the original, then after callbacks.
  See [hooks](../reference/lua/hooks.md) and [Codegen](codegen/codegen.md).
- **ImGui draws.** A script registers with `imgui.onDraw`. The draw scheduler runs callbacks each frame inside an ImGui frame.
  See [ImGui draw scheduler](internals/imgui-draw-scheduler.md).
- **Viewports.** Scripts load meshes, add them to `gd3d.ViewportFrame`, and parent the sprite in the scene graph.
  See [gd3d](../reference/lua/gd3d.md) for the rendering model.

## Threading

The runtime is single threaded. Almost every call must run on the main thread.
See [Getting started](../getting-started/overview.md) for the user-facing rule
and the [C++ API reference](../reference/cpp/api-reference.md) for host API rules.

## Related

- [Getting started](../getting-started/overview.md)
- [Runtime](internals/runtime.md)
- [Bindings framework](internals/bindings-framework.md)
- [Codegen](codegen/codegen.md)
- [CCArray methods](codegen/cc-array.md)
- [Crash sidecar](internals/crash-sidecar.md)
- [Release-hook safety](internals/release-hook-safety.md)
- [hooks](../reference/lua/hooks.md)
- [gd3d](../reference/lua/gd3d.md)
- [C++ API reference](../reference/cpp/api-reference.md)

## Source

- `include/NativeRegistration.hpp`
- `src/main.cpp`
- `src/api.cpp`
- `src/core/Runtime.cpp`
- `src/render3d/viewport/CCViewportFrame.cpp`
- `src/render3d/gpu/GpuSessionDisable.cpp`
- `src/render3d/gpu/Renderer3D.cpp`
- `tools/luau_codegen/emit/cxx_templates.py`
- `src/bindings/imgui/ImGuiCore.cpp`
- `src/bindings/imgui/ImGuiDrawScheduler.hpp`
