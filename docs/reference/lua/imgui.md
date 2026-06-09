# imgui

## Summary

The `imgui` library lets a script draw a [Dear ImGui](https://github.com/ocornut/imgui) overlay on top of the game,
backed by [gd-imgui-cocos](https://github.com/matcool/gd-imgui-cocos). It is debug and tooling UI, not player-facing UI.
Types match `tools/luau_codegen/extra_bindings/imgui.dluau`.

## Model

Use `imgui.onDraw` to set a draw callback.
The runtime runs it every frame. Build windows and widgets inside the callback.
ImGui is immediate mode and saves no widget state, so widgets return their value each frame.
Store values in your script if you want to keep them.

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
    end)
end)

-- later
handle:cancel()
```

## Types

```lua
type ImGuiDrawHandle = { cancel: (self: ImGuiDrawHandle) -> () }
type ImGuiVec2 = { x: number, y: number }
type ImGuiWindowOpts = { size: ImGuiVec2?, pos: ImGuiVec2?, flags: number?, closable: boolean? }
type ImGuiChildOpts = { size: ImGuiVec2? }
type ImGuiButtonOpts = { size: ImGuiVec2? }
```

## Lifecycle and visibility

```lua
imgui.onDraw(fn: () -> ()) -> ImGuiDrawHandle
imgui.cancel(handle: ImGuiDrawHandle) -> ()
imgui.setVisible(visible: boolean) -> ()
imgui.toggle() -> ()
imgui.isVisible() -> boolean
```

`imgui.onDraw` registers a per-frame callback and returns a handle. The first call starts the ImGui backend.
`imgui.cancel` or `handle:cancel()` removes it, and dropping the handle cancels it on GC.
A callback that errors is removed automatically. While hidden, draw callbacks do not run.

## Containers

```lua
imgui.window(title: string, fn: () -> (), opts: ImGuiWindowOpts?) -> boolean
imgui.child(id: string, fn: () -> (), opts: ImGuiChildOpts?) -> ()
```

Both run `fn` then always close the region, even if `fn` errors, so the frame stays balanced.
`imgui.window` returns whether the window wants to stay open.
With `opts.closable` true, the window shows a close button and returns false the frame it is pressed.
`opts.size` and `opts.pos` apply on first use, `opts.flags` is a raw ImGui flags number.

## Widgets

```lua
imgui.text(text: string) -> ()
imgui.button(label: string, opts: ImGuiButtonOpts?) -> boolean
imgui.checkbox(label: string, value: boolean) -> boolean
imgui.sliderFloat(label: string, value: number, min: number, max: number, fmt: string?) -> number
imgui.sliderInt(label: string, value: number, min: number, max: number) -> number
imgui.inputText(label: string, value: string, maxLen: number?) -> string
imgui.inputTextMultiline(label: string, value: string, size: ImGuiVec2?, maxLen: number?) -> string
```

`imgui.text` draws the string verbatim, so `%` is safe. `imgui.button` returns true the frame it is clicked.
`imgui.checkbox` and the sliders return their new value, which you store and pass back next frame.
`fmt` defaults to `"%.3f"`. `maxLen` is the buffer size, default `16384`, capped at `65536`.

## Layout helpers

```lua
imgui.sameLine(offset: number?, spacing: number?) -> ()
imgui.separator() -> ()
imgui.spacing() -> ()
imgui.getContentRegionAvail() -> ImGuiVec2
```

`imgui.sameLine` keeps the next widget on the current line.
`imgui.getContentRegionAvail` returns the space left in the current window as `{ x, y }` in pixels.
Negative sizes also fill the space.

## Limits

- The most draw callbacks at once is `256`. Going over raises an error.
- Each draw callback runs with a `16 ms` budget (`kImGuiScriptDeadlineMs`).
- Widget and window functions only work on the main thread inside an `imgui.onDraw` callback.
  Calling them elsewhere raises an error.
- A draw callback that errors is removed so it does not spam the log every frame.

## Related

- [UI and layouts](ui.md)
- [ImGui draw scheduler](../../contributor/internals/imgui-draw-scheduler.md)
- [Limits and errors](../cpp/limits-and-errors.md)

## Source

- `tools/luau_codegen/extra_bindings/imgui.dluau`
- `src/lua/bindings/imgui/ImGuiBinding.cpp`
- `src/lua/bindings/imgui/ImGuiDrawScheduler.cpp`
- `src/lua/bindings/imgui/ImGuiHost.cpp`
- `src/lua/Config.hpp`
