# Crash sidecar

## Summary

When Luau-driven native code faults, Geode's crashlog shows C++ frames only.
It does not name the hook, binding, script, or mod that was active.
LuauAPI writes `luauapi-last-context.txt` in the crashlogs dir next to Geode's logs.
Read it with the main crashlog to see the Luau side of the fault.

## How to use it

Use this when the game hard-crashes, not when Lua prints an error from a caught script failure.

1. Open the Geode crashlog for the fault address and C++ stack.
2. Open `luauapi-last-context.txt` in the same crashlogs folder.
3. Read `Active boundary` for the innermost hook, binding, or script.
4. Read `Luau stack` for your `@file:line` callback before native code ran.
5. Read `Call chain` for nested boundaries, innermost first.
6. Match `callback_id` to the hook callback when several hooks share one target.

Caught Lua errors from `protectedCall` stay in the Geode log. They do not update the sidecar.

## Contract

- LuauAPI does not install a crash handler. Geode's loader dumps C++ frames at fault time.
- The sidecar must be on disk before the fault. LuauAPI only runs before native code can crash.
- The C++ fault address stays in the main crashlog, not the sidecar.

## File shape

**This example matches a real fault.**
An after-hook on `MenuLayer:init` calls `addChild`, and `libcocos2d.dll` crashes on the native call.

The force crash script:

```lua
geode.hook("geode.gd.MenuLayer:init/0", {
    after = function(self, result)
        return false -- supposed to return result or true
    end,
})
```

The sidecar log:

```text
=== LuauAPI Context ===
timestamp: 2026-06-23T17:40:10Z
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
    Bootstrap.luau:2 in after

=== Call chain (innermost first) ===
#0 hook-after  geode.gd.MenuLayer:init/0  mod=imes.luauapi  cb=1

=== Notes ===
Recorded immediately before native code ran. C++ fault address is in the main crashlog.
```

The Luau stack shows the pending callback (`Bootstrap.luau:2 in after`) captured before `lua_pcall` runs the hook body.
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
3. **Universal choke point.** `Runtime::protectedCallImpl` records script, task, imgui, websocket, web,
   and delegate labels from the `context` string each caller passes.
   Nested ImGui draw closures pass `{.record = false}` so only the top-level `imgui.draw` callback pushes a boundary.
   A `BoundaryScope` pops when the call returns.

Task, imgui, websocket, web, and delegate paths route through `Runtime::protectedCall` without extra site instrumentation.
Generated bindings record separately because they run native code inside an outer `protectedCall`
but are not themselves a `protectedCall` site.

## Stack capture

- **Scripts and hooks:** before `lua_pcall`, record the pending function on the stack (`@file` or chunk name, plus the function name).
- **Bindings:** at CFunction entry, walk the full Luau call stack and refresh it right before native code runs.
- Pre-flush refresh replaces the stack only when the VM walk returns frames.
  Pending-call snapshot from hook or script entry is kept otherwise.

## Flush policy

- In-memory state updates on every boundary push or pop.
- Flush to disk on boundary push when the active boundary changes.
- Flush on the task scheduler interval.
- Flush before generated binding native code runs (`recordBindingEntry` force flush).
- Pop does not flush. The next push, binding entry, or task tick writes the restored outer frame.
- Nested ImGui widget closures (`imgui.window`, `imgui.style.with`, and similar) do not push boundaries.
  Only the top-level `imgui.draw` callback records.
- Before writing, refresh the active Luau stack from the VM.
  Skip disk when the semantic payload is unchanged from the last write.
  Semantic payload is everything except the `timestamp:` line.
- When the boundary stack is empty, disk is not updated. The last non-empty snapshot stays on disk.
- Writes use `luauapi-last-context.tmp`, then rename to `luauapi-last-context.txt`.
  A crash mid-write keeps the prior file or leaves nothing.
- I/O errors are swallowed. The sidecar must not wedge the tick or crash the runtime.

See [Limits and errors](../../reference/cpp/limits-and-errors.md) for flush interval and stack depth caps.

## Threading

The Luau VM is main-thread only. See [Runtime](runtime.md).
Websocket, web, and task callbacks queue to the main thread before Lua runs.
Boundary pushes and flushes stay on the main thread with no locking.

## Toggle

Recording is off by default. Turn on **Enable Crash Context File** (`enable-crash-sidecar`) in LuauAPI mod settings under User Settings.
The toggle applies immediately without a restart.

`luax::diag::setRecordingEnabled(false)` turns off all recording at the API level.
The gate is checked at the top of every record path.

See [hooks](../../reference/lua/hooks.md) for when the sidecar helps with native hook crashes.

## Empty mod context

`currentMod()` comes from the active resources root. See [mod](../../reference/lua/mod.md).
It is null outside a script scope. The sidecar still writes. Mod fields show as `(none)`.

## Log prefix

`print`, `warn`, and Luau panic log as `[<mod.id>]` instead of `[lua]`.
When no mod is in scope the prefix falls back to `[lua]`.
Panic uses `[<mod.id>:panic code=N]`.

## Related

- [Runtime](runtime.md)
- [Task scheduler](task-scheduler.md)
- [Limits and errors](../../reference/cpp/limits-and-errors.md)
- [Architecture](../architecture.md)
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
- `src/bindings/imgui/ImGuiBindingInternal.hpp`
- `tools/luau_codegen/emit/cxx_templates.py`
- `tools/luau_codegen/emit/bindings/class_file.py`
- `tools/luau_codegen/emit/bindings/free_functions.py`
