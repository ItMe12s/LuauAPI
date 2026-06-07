# Architecture

## Summary

This page gives the big picture for contributors. It names the main parts and traces how a script gets from a file to running code.

## The parts

- Public C++ API in `imes::luauapi`. The host facing surface. See [API reference](../cpp/api-reference.md).
- Runtime. Owns the Lua state, memory, deadlines, and the bytecode cache. See [Runtime](runtime.md).
- Bindings framework. Exposes C++ types to Lua. See [Bindings framework](bindings-framework.md).
- Module system. Implements sandboxed `require`. See [Module system](module-system.md).
- Task scheduler. Drives `task` callbacks on the game tick. See [Task scheduler](task-scheduler.md).
- ImGui draw scheduler. Drives `imgui.onDraw` callbacks each frame. See [ImGui draw scheduler](imgui-draw-scheduler.md).
- Codegen. Generates the game bindings and the type stubs. See [Codegen](codegen.md).

## Source layout

- `include/LuauAPI.hpp`: the public header.
- `src/api.cpp`: the public API implementation.
- `src/main.cpp`: the mod entry points that drive the runtime lifecycle.
- `src/lua/Config.hpp`: the limits and deadlines.
- `src/lua/runtime/`: the runtime and the memory allocator.
- `src/lua/bindings/`: the binding registry, the framework, task, imgui, and geode bindings.
- `src/lua/bindings/framework/`: usertypes, the stack, references, and fields.
- `src/lua/bindings/geode/`: handwritten `geode.*` bindings (`fs`, `json`, `Mod`, `utils.web`,
  `utils.base64`, `utils.permission`, `cocos`, `ColorProvider`, `Keybind`, `VersionInfo`). See
  [bindings-framework.md](bindings-framework.md#handwritten-bindings) for the file map.
- `src/lua/bindings/imgui/`: ImGui binding and draw scheduler.
- `src/lua/module/`: the requirer and the path rules.
- `src/lua/util/`: small data structures such as the indexed slot map used by the schedulers.
- `build/luauapi-gen/src/`: generated C++ bindings from codegen.
- `tools/luau_codegen/`: the Python code generator.
- `tests/`: the host tests.

## Lifecycle

The runtime follows the mod and game lifecycle, wired in `src/main.cpp`:

- The main thread id is recorded at mod load.
- The runtime is created when the mod is loaded.
- The runtime shuts down when the game is exiting.

## How a script runs

This is the path for `runFile`:

1. The host calls `runFile` on the main thread.
2. `src/api.cpp` checks the thread, then resolves the path inside the resources root and reads the file.
3. It builds a chunk name and sets the resources root scope.
4. The runtime compiles the source to bytecode, or reuses a cached copy.
5. The runtime loads the bytecode and runs it inside a protected call with a deadline.
6. Errors are caught and stored as the last error, and the result is returned to the host.

## How a hook runs

1. A script registers a callback with `geode.hook`.
2. The generated hook function for that game method is installed.
3. When the game calls the method, the generated function runs the `before` callbacks, then the original unless skipped, then the `after` callbacks.

## How ImGui draw runs

1. A script registers a callback with `imgui.onDraw`.
2. `ImGuiDrawScheduler` stores the callback and runs it each frame inside an ImGui frame.
3. The callback must finish within the ImGui deadline. See [ImGui draw scheduler](imgui-draw-scheduler.md).

## Threading

The runtime is single threaded. Almost every call must run on the main thread, and the runtime enforces this.
The async API performs its file work off thread, then hops to the main thread to run the script.

## Source

- `src/main.cpp`
- `src/api.cpp`
- `src/lua/runtime/Runtime.cpp`
- `tools/luau_codegen/emit/cxx_templates.py`
- `src/lua/bindings/imgui/ImGuiDrawScheduler.cpp`
