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
`imgui.window`, `imgui.child`, tabs, popups, tables, menus, groups, and style helpers are scoped wrappers.
They call the matching ImGui begin or push function, run the Lua closure through the runtime protected call,
then always call the matching end or pop function. So a script error cannot skip the matching cleanup.
There are no bare `begin` or `end` functions on purpose.

The binding is split by feature:

- `ImGuiBindingInternal.hpp` has shared guards and argument helpers.
- `ImGuiWidgets.cpp` has common widgets and input helpers.
- `ImGuiLayout.cpp` has layout, groups, and tree nodes.
- `ImGuiPopups.cpp` has tabs, popups, and tooltips.
- `ImGuiTables.cpp` has table helpers.
- `ImGuiMenus.cpp` has menu helpers.
- `ImGuiStyle.cpp` has style and theme helpers.
- `ImGuiConstants.cpp` has enum tables.

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

Draw callback count and per-callback deadline caps are in limits-and-errors.

See [Limits and errors](../../reference/cpp/limits-and-errors.md).

## Related

- [imgui reference](../../reference/lua/imgui.md)
- [Task scheduler](task-scheduler.md)
- [Runtime](runtime.md)

## Source

- `src/bindings/imgui/ImGuiBinding.cpp`
- `src/bindings/imgui/ImGuiBindingInternal.hpp`
- `src/bindings/imgui/ImGuiWidgets.cpp`
- `src/bindings/imgui/ImGuiLayout.cpp`
- `src/bindings/imgui/ImGuiPopups.cpp`
- `src/bindings/imgui/ImGuiTables.cpp`
- `src/bindings/imgui/ImGuiMenus.cpp`
- `src/bindings/imgui/ImGuiStyle.cpp`
- `src/bindings/imgui/ImGuiConstants.cpp`
- `src/bindings/imgui/ImGuiDrawScheduler.hpp`
- `src/bindings/imgui/ImGuiDrawScheduler.cpp`
- `src/bindings/imgui/ImGuiHost.cpp`
- `src/core/Config.hpp`
