# ImGui draw scheduler

## Summary

The ImGui draw scheduler runs `imgui` draw callbacks every frame. It lives in `src/bindings/imgui/`.
gd-imgui-cocos gives one draw callback, and the scheduler lets many script callbacks share it.
The binding exposes the `imgui` library, the host wires the backend, and the scheduler holds the callbacks and runs them.

## The binding

`ImGuiCore.cpp` builds the `imgui` global table and registers it.
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
- `ImGuiCore.cpp` has entry registration, the draw handle, the scheduler, and the gd-imgui-cocos host.
- `ImGuiWidgetsLayout.cpp` has common widgets, input helpers, and layout or tree helpers.
- `ImGuiPopupsTablesMenus.cpp` has tabs, popups, tooltips, tables, and menus.
- `ImGuiStyleFonts.cpp` has style, theme, enum constants, and font registration.

## The scheduler

`ImGuiDrawScheduler` is a single instance.
Each entry holds an id, a callback reference, and a cancelled flag.
It does not track visibility, since `ImGuiCocos` owns that. `drawAll()` runs once per frame from inside the ImGui frame.
It collects the live ids, runs each callback, and sets an `inFrame` flag while it runs them.
A callback that errors is marked cancelled and removed, so a broken script is dropped instead of logging every frame.
The scheduler only runs Lua callbacks. It never calls ImGui `Begin` or `End` itself.
Only the host draw lambda calls `drawAll()`. There is no `CCNode` tick like the task scheduler has.

## The host

`ImGuiCore.cpp` owns gd-imgui-cocos and is the only binding file that includes `<imgui-cocos.hpp>`.
`initImGuiHost()` is idempotent. It reads the `imgui-scale` mod setting (see [imgui](../../reference/lua/imgui.md)) and listens for live changes.
It sets up ImGuiCocos and registers the draw lambda that calls `drawAll()`.
The setup callback rebuilds the font atlas through `imguiFontRebuildAtlas()` after each ImGui init or reload.
It runs lazily on the first `imgui.onDraw`, so a script that never uses ImGui pays nothing.
If there's no OpenGL view yet, setup waits until available, making early `imgui.onDraw` safe.
All host entry points return early while game textures are unloaded. See [Limits and errors](../../reference/cpp/limits-and-errors.md).
`shutdownImGuiHost()` clears the scheduler, font registry, and tears down the backend.
`src/main.cpp` calls it on game exit before `Runtime::shutdown()`, so the draw lambda detaches before the Lua state closes.
The default input mode stays in place, so the game keeps input unless an ImGui window is hovered or focused.
`imgui.setVisible`, `imgui.toggle`, and `imgui.isVisible` forward to `ImGuiCocos`.
gd-imgui-cocos is vendored in `gd-imgui-cocos/`. See [Building from source](../building.md).

## Limits

Draw callback count, per-callback deadlines, font errors, and GPU session disable are in [Limits and errors](../../reference/cpp/limits-and-errors.md).

## Related

- [Architecture](../architecture.md)
- [imgui](../../reference/lua/imgui.md)
- [Task scheduler](task-scheduler.md)
- [Runtime](runtime.md)
- [Limits and errors](../../reference/cpp/limits-and-errors.md)

## Source

- `src/bindings/imgui/ImGuiCore.cpp`
- `src/bindings/imgui/ImGuiWidgetsLayout.cpp`
- `src/bindings/imgui/ImGuiPopupsTablesMenus.cpp`
- `src/bindings/imgui/ImGuiStyleFonts.cpp`
- `src/bindings/imgui/ImGuiBindingInternal.hpp`
- `src/bindings/imgui/ImGuiDrawScheduler.hpp`
- `src/bindings/imgui/ImGuiFontRegistry.hpp`
- `src/bindings/imgui/ImGuiHost.hpp`
- `src/render3d/gpu/GpuSessionDisable.cpp`
- `gd-imgui-cocos/src/backend.cpp`
- `gd-imgui-cocos/src/hooks.cpp`
- `gd-imgui-cocos/CMakeLists.txt`
- `cmake/ImGui.cmake`
- `cmake/ImGuiCocos.cmake`
- `src/core/Config.hpp`
