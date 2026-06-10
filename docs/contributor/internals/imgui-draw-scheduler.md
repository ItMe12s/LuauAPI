# ImGui draw scheduler

## Summary

The ImGui draw scheduler runs `imgui` draw callbacks every frame. It lives in `src/bindings/imgui/`.
gd-imgui-cocos gives one draw callback, and the scheduler lets many script callbacks share it.
The binding exposes the `imgui` library, the host wires the backend, and the scheduler holds the callbacks and runs them.

## The binding

`ImGuiBinding.cpp` builds the `imgui` global table and registers it.
It registers the handle metatable and adds a shutdown hook that clears the scheduler.
Each callback stores its Lua function as a `LuaRef`.
The handle returned to Lua is a small userdata that holds a draw id and a `cancel` method.

Widget and window functions call `requireFrame` first,
which rejects a call made off the main thread or outside a draw callback.
So a stray `imgui.button` from a `task` callback errors cleanly instead of breaking the ImGui frame.
`imgui.window` and `imgui.child` are scoped wrappers.
They call `Begin` or `BeginChild`, run the Lua closure through the runtime protected call,
then always call `End` or `EndChild`, so a script error cannot skip the matching end.
There are no bare `begin` or `end` functions on purpose.

## The scheduler

`ImGuiDrawScheduler` is a single instance.
Each entry holds an id, a callback reference, and a cancelled flag.
It does not track visibility, since `ImGuiCocos` owns that. `drawAll()` runs once per frame from inside the ImGui frame.
It collects the live ids, runs each callback, and sets an `inFrame` flag while it runs them.
A callback that errors is marked cancelled and removed, so a broken script is dropped instead of logging every frame.
The scheduler only runs Lua callbacks. It never calls ImGui `Begin` or `End` itself.
Only the host draw lambda calls `drawAll()`. There is no `CCNode` tick like the task scheduler has.

## The host

`ImGuiHost.cpp` owns gd-imgui-cocos and is the only file that includes `<imgui-cocos.hpp>`.
`initImGuiHost()` is idempotent. It sets up ImGuiCocos and registers the draw lambda that calls `drawAll()`.
It runs lazily on the first `imgui.onDraw`, so a script that never uses ImGui pays nothing.
`shutdownImGuiHost()` clears the scheduler and tears down the backend.
`src/main.cpp` calls it on game exit before `Runtime::shutdown()`, so the draw lambda detaches before the Lua state closes.
The default input mode stays in place, so the game keeps input unless an ImGui window is hovered or focused.
`imgui.setVisible`, `imgui.toggle`, and `imgui.isVisible` forward to `ImGuiCocos`.

## Limits

Draw callback count and per-callback deadline are capped.
Capacity is enforced on active (non-cancelled) callbacks, not the internal slot vector size.
Cancelled callbacks stay in the vector until the next `drawAll` compacts them, but they no longer count toward the cap.
The binding checks capacity before adding and raises an error when full.
The ImGui deadline is tighter than the hook budget because draw callbacks run every frame.

See [Limits and errors](../../reference/cpp/limits-and-errors.md) for caps and error strings.

## Related

- [imgui reference](../../reference/lua/imgui.md)
- [Task scheduler](task-scheduler.md)
- [Runtime](runtime.md)
- [Limits and errors](../../reference/cpp/limits-and-errors.md)

## Source

- `src/bindings/imgui/ImGuiBinding.cpp`
- `src/bindings/imgui/ImGuiDrawScheduler.hpp`
- `src/bindings/imgui/ImGuiDrawScheduler.cpp`
- `src/bindings/imgui/ImGuiHost.cpp`
- `src/core/Config.hpp`
