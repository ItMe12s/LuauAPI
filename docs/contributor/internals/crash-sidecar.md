# Crash sidecar

## Summary

When Luau-driven native code faults, Geode's crashlog shows C++ frames only.
It does not name the hook, binding, script, or mod that was active.
LuauAPI writes `luauapi-last-context.txt` in the crashlogs dir next to Geode's logs.
Read it with the main crashlog to see the Luau side of the fault.

## Contract

- LuauAPI does not install a crash handler. Geode's loader dumps C++ frames at fault time.
- The sidecar must be on disk before the fault. LuauAPI only runs before native code can crash.
- The C++ fault address stays in the main crashlog, not the sidecar.

## File shape

This example matches a real fault, an after-hook on `MenuLayer:init` calls `addChild`, and `libcocos2d.dll` crashes on the native call.

The force crash script:

```lua
geode.hook("geode.gd.MenuLayer:init/0", {
    after = function(self, result)
        return false
    end,
})
```

The sidecar log:

```text
=== LuauAPI Context ===
timestamp: 2026-06-23T15:26:39Z
runtime_status: Ready
geode_version: v5.7.1
luauapi_version: v0.1.0-beta.9

=== Active boundary (innermost) ===
kind: hook-after
target: geode.gd.MenuLayer:init/0
hook_kind: after
callback_id: 1
mod_id: imes.luauapi
mod_version: v0.1.0-beta.9
resources_root: C:\Program Files (x86)\Steam\steamapps\common\Geometry Dash\geode\unzipped\imes.luauapi\resources\imes.luauapi

=== Luau stack ===
    Bootstrap.luau:3 in after

=== Call chain (innermost first) ===
#0 hook-after  geode.gd.MenuLayer:init/0  mod=imes.luauapi  cb=1

=== Notes ===
Recorded immediately before native code ran. C++ fault address is in the main crashlog.
```

The Luau stack shows the pending callback (`Bootstrap.luau:3 in after`) captured before `lua_pcall` runs the hook body.
When the fault happens inside a generated binding,
the call chain gains a `generated-binding` frame at `#0` and the stack lists full `@file:line` frames.

Pair the sidecar with the crashlog stack.
In the example above, `CCNode::addChild` in `libcocos2d.dll` lines up with `self:addChild(label)` in the hook callback.

## Where boundaries are recorded

Three layers:

1. **Hooks.** Generated `runLuaPreHooks` and `runLuaPostHooks` pass hook kind and callback id into `Runtime::protectedCall`.
   See [Codegen](../codegen/codegen.md).
2. **Generated bindings.** Each generated method and free function calls `recordBindingEntry` at CFunction entry.
   That returns a `BoundaryScope` that pops when the binding returns.
3. **Universal choke point.** `Runtime::protectedCallImpl` records `script` and task,
   imgui, websocket, web, and delegate labels from the `context` string each caller passes.
   A `BoundaryScope` pops when the call returns.

Task, imgui, websocket, web, and delegate paths route through `Runtime::protectedCall` without extra site instrumentation.
Generated bindings record separately because they run native code inside an outer `protectedCall`
but are not themselves a `protectedCall` site.

## Stack capture

- **Scripts and hooks:** before `lua_pcall`, record the pending function on the stack (`@file` or chunk name, plus the function name).
- **Bindings:** at CFunction entry, walk the full Luau call stack and refresh it right before native code runs.

## Flush policy

- In-memory state updates on every boundary push or pop.
- Flush to disk when the active boundary changes or before generated binding native code runs.
- The task scheduler tick also flushes on a change-or-interval throttle.
- When the boundary stack is empty, disk is not updated. The last non-empty snapshot stays on disk.
- Writes use `luauapi-last-context.tmp`, then rename to `luauapi-last-context.txt`.
  A crash mid-write keeps the prior file or leaves nothing.
- I/O errors are swallowed. The sidecar must not wedge the tick or crash the runtime.

Flush timing and stack depth caps live in `src/core/Config.hpp`.

## Threading

The Luau VM is main-thread only. See [Runtime](runtime.md).
Websocket, web, and task callbacks queue to the main thread before Lua runs.
Boundary pushes and flushes stay on the main thread with no locking.

## Toggle

`luax::diag::setRecordingEnabled(false)` turns off all recording.
The gate is checked at the top of every record path.
The imgui per-frame draw path is the hottest push site.
Disable recording there if profiling shows the stack walk is too costly.

## Empty mod context

`currentMod()` comes from the active resources root. See [mod](../../reference/lua/mod.md).
It is null outside a script scope. The sidecar still writes. Mod fields show as `(none)`.

## Log prefix

`print`, `warn`, and Luau panic log as `[<mod.id>]` instead of `[lua]`.
When no mod is in scope the prefix falls back to `[lua]`.
Panic uses `[<mod.id>:panic code=N]`.

## Related

- [Architecture](../architecture.md)
- [Runtime](runtime.md)
- [Task scheduler](task-scheduler.md)
- [Codegen](../codegen/codegen.md)
- [hooks](../../reference/lua/hooks.md)
- [mod](../../reference/lua/mod.md)

## Source

- `src/diagnostics/BoundaryRecorder.hpp`
- `src/diagnostics/BoundaryRecorder.cpp`
- `src/core/StackFormat.hpp`
- `src/core/StackFormat.cpp`
- `src/core/Runtime.cpp`
- `src/core/Config.hpp`
- `src/bindings/task/TaskScheduler.cpp`
- `tools/luau_codegen/emit/cxx_templates.py`
- `tools/luau_codegen/emit/bindings/class_file.py`
- `tools/luau_codegen/emit/bindings/free_functions.py`
