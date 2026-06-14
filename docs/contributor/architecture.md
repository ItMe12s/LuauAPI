# Architecture

## Summary

The big picture for contributors.
This page names the main parts and traces how a script gets from a file to running code.

## The parts

- Public C++ API in `imes::luauapi`. The host-facing surface. See [API reference](../reference/cpp/api-reference.md).
- Runtime. Owns the Lua state, memory, deadlines, and the bytecode cache. See [Runtime](internals/runtime.md).
- Bindings framework. Exposes C++ types to Lua. See [Bindings framework](internals/bindings-framework.md).
- Module system. Implements sandboxed `require`. See [Module system](internals/module-system.md).
- Task scheduler. Drives `task` callbacks on the game tick. See [Task scheduler](internals/task-scheduler.md).
- ImGui draw scheduler. Drives `imgui.onDraw` callbacks each frame.
  See [ImGui draw scheduler](internals/imgui-draw-scheduler.md).
- WebSocket. Client and local server bindings backed by IXWebSocket. See [websocket](../reference/lua/websocket.md).
- 3D rendering. Loads glTF meshes and draws them through `gd3d.ViewportFrame` nodes. See [gd3d](../reference/lua/gd3d.md).
- Codegen. Generates the game bindings and the type stubs. See [Codegen](codegen/codegen.md).

## Repository layout

- `include/LuauAPI.hpp`: the public header.
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
- `src/bindings/render3d/`: handwritten `gd3d` bindings.
- `src/render3d/`: glTF loading, GPU rendering, and the `CCViewportFrame` node.
- `src/require/`: the requirer and the path rules.
- `build/luauapi-gen/src/`: generated C++ bindings from codegen.
- `tools/luau_codegen/`: the Python code generator.
- `tests/`: the host tests.

## Lifecycle

The runtime follows the mod and game lifecycle, wired in `src/main.cpp`.
The main thread id is recorded at mod load, the runtime is created when the mod is loaded,
and it shuts down when the game is exiting.

## How a script runs

The host calls `runFile` on the main thread. The runtime resolves the path, compiles or loads cached bytecode, and runs it under a deadline.
Errors are caught and returned to the host.
See [Runtime](internals/runtime.md) for the full pipeline.

## How a hook runs

A script registers with `geode.hook`. Generated hook code runs before callbacks, the original, then after callbacks.
See [hooks](../reference/lua/hooks.md) and [Codegen](codegen/codegen.md).

## How ImGui draw runs

A script registers with `imgui.onDraw`. The draw scheduler runs callbacks each frame inside an ImGui frame.
See [ImGui draw scheduler](internals/imgui-draw-scheduler.md).

## How a ViewportFrame draws

Scripts load meshes, add them to `gd3d.ViewportFrame`, and parent the node in the scene graph.
See [gd3d](../reference/lua/gd3d.md) for the rendering model.

## Threading

The runtime is single threaded. Almost every call must run on the main thread.
See [Getting started Key concepts](../getting-started/overview.md) for the user-facing rule.

## Related

- [Getting started overview](../getting-started/overview.md)
- [Runtime](internals/runtime.md)
- [Bindings framework](internals/bindings-framework.md)
- [Module system](internals/module-system.md)
- [Task scheduler](internals/task-scheduler.md)
- [ImGui draw scheduler](internals/imgui-draw-scheduler.md)
- [Codegen](codegen/codegen.md)
- [Pair containers](codegen/pair-containers.md)
- [Nested containers](codegen/nested-containers.md)
- [ccCArray fields](codegen/cc-c-array.md)
- [Platform parity](codegen/platform-parity.md)
- [hooks](../reference/lua/hooks.md)
- [gd3d](../reference/lua/gd3d.md)
- [API reference](../reference/cpp/api-reference.md)

## Source

- `src/main.cpp`
- `src/api.cpp`
- `src/core/Runtime.cpp`
- `src/render3d/viewport/CCViewportFrame.cpp`
- `src/render3d/gpu/Renderer3D.cpp`
- `tools/luau_codegen/emit/cxx_templates.py`
- `src/bindings/imgui/ImGuiDrawScheduler.cpp`
