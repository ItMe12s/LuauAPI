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
- ImGui draw scheduler. Drives `imgui.onDraw` callbacks each frame. See [ImGui draw scheduler](internals/imgui-draw-scheduler.md).
- 3D rendering. Loads glTF meshes and draws them through `gd3d.ViewportFrame` nodes. See [gd3d](../reference/lua/gd3d.md).
- Codegen. Generates the game bindings and the type stubs. See [Codegen](codegen/codegen.md).

## Source layout

- `include/LuauAPI.hpp`: the public header.
- `src/api.cpp`: the public API implementation.
- `src/main.cpp`: the mod entry points that drive the runtime lifecycle.
- `src/core/Config.hpp`: the limits and deadlines. See [Limits and errors](../reference/cpp/limits-and-errors.md).
- `src/core/`: the runtime, memory allocator, and small utilities such as the indexed slot map.
- `src/framework/`: the binding registry, usertypes, stack interop, callbacks, views, and scheduling.
- `src/bindings/geode/`: handwritten `geode.*` bindings (web cluster in `web/`).
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

For `runFile`:

1. The host calls `runFile` on the main thread.
2. `src/api.cpp` checks the thread, then resolves the path inside the resources root and reads the file.
3. It builds a chunk name and sets the resources root scope.
4. The runtime compiles the source to bytecode, or reuses a cached copy.
5. The runtime loads the bytecode and runs it inside a protected call with a deadline.
6. Errors are caught and stored as the last error, and the result is returned to the host.

## How a hook runs

A script registers with `geode.hook`. Generated hook code runs before callbacks, the original, then after callbacks.
See [Hooks](../reference/lua/hooks.md) and [Codegen](codegen/codegen.md).

## How ImGui draw runs

A script registers with `imgui.onDraw`. The draw scheduler runs callbacks each frame inside an ImGui frame.
See [ImGui draw scheduler](internals/imgui-draw-scheduler.md).

## How a ViewportFrame draws

Scripts load meshes, add them to `gd3d.ViewportFrame`, and parent the node in the scene graph.
Each frame the node renders off-screen and blits into its content rect.
See [gd3d](../reference/lua/gd3d.md) Rendering model.

## Threading

The runtime is single threaded. Almost every call must run on the main thread, and the runtime enforces this.
The async API does its file work off thread, then hops to the main thread to run the script.

## Related

- [Runtime](internals/runtime.md)
- [Bindings framework](internals/bindings-framework.md)
- [Codegen](codegen/codegen.md)
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
