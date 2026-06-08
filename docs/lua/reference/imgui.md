# Reference: imgui

## Summary

This page is the type reference for the `imgui` library.
It lets a script draw a [Dear ImGui](https://github.com/ocornut/imgui) overlay on top of the game,
backed by [gd-imgui-cocos](https://github.com/matcool/gd-imgui-cocos).
The types match `tools/luau_codegen/extra_bindings/imgui.dluau`, merged into `types/geode.d.luau` at build time.

ImGui here is debug and tooling UI. It is not meant for player-facing, in-game UI.

## Model

You register a draw callback with `imgui.onDraw`. The runtime calls it every frame inside an ImGui frame.
You build windows and widgets from inside that callback.
ImGui is immediate mode. There is no stored widget tree.
A widget that holds a value, like a checkbox or a slider or a text box, returns its new value each frame. Your script stores it.

```lua
local state = { enabled = false, speed = 1.0 }

local handle = imgui.onDraw(function()
    imgui.window("LuauAPI demo", function()
        imgui.text("Hello from Luau")
        if imgui.button("Click me") then
            print("clicked")
        end
        state.enabled = imgui.checkbox("Enable", state.enabled)
        state.speed = imgui.sliderFloat("Speed", state.speed, 0, 10)
        imgui.separator()
    end)
end)

-- later
handle:cancel()
imgui.setVisible(false)
imgui.toggle()
```

## Types

```lua
type ImGuiDrawHandle = {
    cancel: (self: ImGuiDrawHandle) -> (),
}

type ImGuiVec2 = { x: number, y: number }

type ImGuiWindowOpts = { size: ImGuiVec2?, pos: ImGuiVec2?, flags: number?, closable: boolean? }
type ImGuiChildOpts = { size: ImGuiVec2? }
type ImGuiButtonOpts = { size: ImGuiVec2? }
```

## Functions

### imgui.onDraw

```lua
imgui.onDraw(fn: () -> ()) -> ImGuiDrawHandle
```

Registers a per frame draw callback and returns a handle. The first call also starts the ImGui backend.
Build your windows and widgets from inside `fn`.

### imgui.cancel

```lua
imgui.cancel(handle: ImGuiDrawHandle) -> ()
```

Removes a draw callback. You can also call `handle:cancel()`. A callback that raises an error is removed automatically.
Dropping the handle also cancels the callback when Lua collects the handle userdata.

### imgui.setVisible / imgui.toggle / imgui.isVisible

```lua
imgui.setVisible(visible: boolean) -> ()
imgui.toggle() -> ()
imgui.isVisible() -> boolean
```

Show, hide, or query the overlay. While hidden, draw callbacks do not run.

### imgui.window

```lua
imgui.window(title: string, fn: () -> (), opts: ImGuiWindowOpts?) -> boolean
```

Opens a window, runs `fn` to draw its contents, then closes it.
The window is always closed even if `fn` errors, so the frame stays balanced. Returns whether the window wants to stay open.
When `opts.closable` is true the window shows a close button and the return value becomes false the frame the button is pressed.
`opts.size` and `opts.pos` apply on first use, `opts.flags` is a raw ImGui window flags number.

### imgui.child

```lua
imgui.child(id: string, fn: () -> (), opts: ImGuiChildOpts?) -> ()
```

Draws a child region. Like `imgui.window`, the region is always ended even if `fn` errors.

### imgui.text

```lua
imgui.text(text: string) -> ()
```

Draws a text label. The string is drawn verbatim, so `%` is safe.

### imgui.button

```lua
imgui.button(label: string, opts: ImGuiButtonOpts?) -> boolean
```

Draws a button. Returns true the frame it is clicked.

### imgui.checkbox

```lua
imgui.checkbox(label: string, value: boolean) -> boolean
```

Draws a checkbox and returns its new state. Store the result and pass it back next frame.

### imgui.sliderFloat / imgui.sliderInt

```lua
imgui.sliderFloat(label: string, value: number, min: number, max: number, fmt: string?) -> number
imgui.sliderInt(label: string, value: number, min: number, max: number) -> number
```

Draws a slider and returns its new value. `fmt` is an optional printf style format for the float slider and defaults to `"%.3f"`.

### imgui.inputText

```lua
imgui.inputText(label: string, value: string, maxLen: number?) -> string
```

Draws a single line text box and returns its new contents. `maxLen` is the buffer size and defaults to `16384`, capped at `65536`.

### imgui.inputTextMultiline

```lua
imgui.inputTextMultiline(label: string, value: string, size: ImGuiVec2?, maxLen: number?) -> string
```

Draws a multi line text box and returns its new contents. Use it for code or long text.
`size` is the box size in pixels. A zero value lets ImGui pick the size. `maxLen` is the buffer size and defaults to `16384`, capped at `65536`.

### imgui.sameLine / imgui.separator / imgui.spacing

```lua
imgui.sameLine(offset: number?, spacing: number?) -> ()
imgui.separator() -> ()
imgui.spacing() -> ()
```

Layout helpers. `imgui.sameLine` keeps the next widget on the current line.

### imgui.getContentRegionAvail

```lua
imgui.getContentRegionAvail() -> ImGuiVec2
```

Returns the space left in the current window as `{ x, y }` in pixels.
Use it to size widgets to the window. Negative sizes also fill the space, typically used to split rows.

## Limits

- The most draw callbacks at once is `256`. Going over raises an error.
- Each draw callback runs with a `16 ms` budget (`kImGuiScriptDeadlineMs`). Heavy UI across many callbacks can still cost frame time.
- Widget and window functions only work on the main thread, inside an `imgui.onDraw` callback. Calling them elsewhere (for example from a `task.every` callback) raises an error.
- A draw callback that errors is removed so it does not spam the log every frame.

## Related

- [UI and layouts](../guide/ui-and-layouts.md)
- [ImGui draw scheduler](../../dev/imgui-draw-scheduler.md)
- [Limits and errors](../../cpp/limits-and-errors.md)

## Source

- `tools/luau_codegen/extra_bindings/imgui.dluau`
- `src/lua/bindings/imgui/ImGuiBinding.cpp`
- `src/lua/bindings/imgui/ImGuiDrawScheduler.cpp`
- `src/lua/bindings/imgui/ImGuiHost.cpp`
- `src/lua/Config.hpp`
