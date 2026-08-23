# imgui

## Summary

`imgui` draws Dear ImGui UI over the game through gd-imgui-cocos.
Use it for player mod menus, settings panels, tabs, popups, and debug tools.

## Model

### Registration and lifecycle

Register one draw callback with `imgui.onDraw`. Build all ImGui UI inside that callback.
Widget calls must run inside `imgui.onDraw`.
See [Getting started](../../getting-started/overview.md) for the shared main-thread rule.

`onDraw` starts the backend on first use. `cancel` removes a callback.
Dropping the handle cancels it on GC. A callback that errors is removed.
Use `setVisible`, `toggle`, and `isVisible` to show or hide the overlay without unregistering.

A minimal closable window:

```lua
local open = true
local enabled = true

imgui.onDraw(function()
    if not open then return end

    open = imgui.window("my.mod/quick", function()
        enabled = imgui.checkbox("Enabled", enabled)
    end, { closable = true })
end)
```

An input text round-trip (the shared buffer means you save the return every frame):

```lua
local name = ""

imgui.onDraw(function()
    name = imgui.inputText("Name", name, 64)
end)
```

### Immediate mode

ImGui is immediate mode. It does not store your widget state.
Pass current values each frame and save returned values in Luau.

The game keeps input when ImGui is not hovered or focused.
Focused ImGui windows capture the input they need.

### Scoped wrappers

`window`, `child`, `group`, `tabBar`, `tabItem`, `popup`, `popupModal`, `table`, `menuBar`, `menu`, `style.with`,
and `font.with` always close their ImGui region after `fn`, even when `fn` errors.

- `window` returns false when its close button is pressed. Set `closable = true` to get the close button.
- All mods share one ImGui context. Identical window titles collide silently, so prefix windows with your mod id.
- Use `sizeCond` or `posCond` with `imgui.Cond.Always` for animated windows.

### Widgets, buffers, and indexes

- Text is drawn as raw text, so percent signs are safe.
- Input text uses a shared per-thread buffer with a length cap. See [Limits and errors](../cpp/limits-and-errors.md).
- Combo and list indexes are zero-based. Colors use `{ x, y, z, w }` floats from 0 to 1.
- `treeNode` returns a tracked open value only when `opts.open` is set.
- `tabItem` and `popupModal` return a close state only when `closable` is true.
- Use `openPopup` before `popup` or `popupModal`.
- `imgui.tooltip(fn)` shows a tooltip for the previous item only when it is hovered.
- Call table row and column helpers inside `imgui.table`.
- `menuItem` returns clicked when no selected value is passed. When selected is passed, it returns the new selected value.

### Performance and unsupported APIs

Use one `imgui.onDraw` per mod when possible.
Do not do IO, web work, or heavy list rebuilds in `onDraw`.
Image, texture, docking, viewport, and draw list APIs are not bound.

## Example

```lua
local open = true
local enabled = true
local volume = 0.7
local mode = 1
local modes = { "Easy", "Normal", "Hard" }
local accent = { x = 0.3, y = 0.7, z = 1.0, w = 1.0 }

imgui.onDraw(function()
    if not open then return end

    imgui.theme.apply("dark")

    open = imgui.window("my.mod/settings", function()
        imgui.style.with({
            frameRounding = 4,
            colors = {
                [imgui.Col.Button] = accent,
            },
        }, function()
            imgui.tabBar("tabs", function()
                imgui.tabItem("General", function()
                    enabled = imgui.checkbox("Enabled", enabled)
                    volume = imgui.sliderFloat("Volume", volume, 0, 1)
                    mode = imgui.combo("Mode", mode, modes)
                    accent = imgui.colorEdit4("Accent", accent)
                end)

                imgui.tabItem("Danger", function()
                    if imgui.button("Reset") then
                        imgui.openPopup("reset-confirm")
                    end

                    imgui.popupModal("reset-confirm", function()
                        imgui.text("Reset settings?")
                        if imgui.button("Yes") then
                            volume = 0.7
                            imgui.closeCurrentPopup()
                        end
                        imgui.sameLine()
                        if imgui.button("No") then
                            imgui.closeCurrentPopup()
                        end
                    end)
                end)
            end)
        end)
    end, {
        closable = true,
        size = { x = 420, y = 280 },
        flags = imgui.Flag.Window.NoCollapse,
    })
end)
```

See [mod/demo/demo_modmenu.luau](../../../mod/demo/demo_modmenu.luau) for a full mod menu.

## Style and theme

`imgui.theme.apply("dark" | "light" | "classic")` and `imgui.theme.applyCustom(opts)` change global ImGui style until changed again.
Use `imgui.style.with(opts, fn)` for per-window or per-mod styling that does not affect other `onDraw` callbacks.
Useful option fields include `alpha`, `windowPadding`, `windowRounding`, `framePadding`, `frameRounding`, `itemSpacing`, and `colors`.
Color keys can be `imgui.Col.*` values or color names from `imgui.Col`.

## Display scale

LuauAPI exposes an ImGui scale slider in its Geode mod settings.
There is no Lua API for this.

## Fonts

Call `imgui.font.add(root, path, size)` outside `imgui.onDraw`. It errors if called inside a draw callback.
Use raw `.ttf` resource files, not GD bitmap fonts. The first argument uses the same roots as [fs](fs.md).
Before the backend initializes, `add` stores the font and builds it at backend init.
`imgui.font.with(font, fn)` must run inside `imgui.onDraw` and always pops after `fn`, even when `fn` errors.
See [Limits and errors](../cpp/limits-and-errors.md) for font error strings.

```lua
local font, err = imgui.font.add("resources", "assets/Inter.ttf", 18)
if not font then
    error(err)
end

imgui.onDraw(function()
    imgui.font.with(font, function()
        imgui.text("Custom font")
    end)
end)
```

## Limits

See [Limits and errors](../cpp/limits-and-errors.md) for draw callback caps,
script deadlines, input text caps, font errors, and GPU session disable.

## Finding signatures

The authoritative argument lists live in the generated type stubs, surfaced as editor autocomplete.
See [type stubs](type-stubs.md).
Handwritten extras are in `tools/luau_codegen/extra_bindings/imgui.dluau`.
Constant tables live under `imgui.Flag.*`, `imgui.Col.*`, `imgui.StyleVar.*`, and `imgui.Cond.*`.
Use constants instead of magic numbers.

## Related

- [UI and layouts](ui.md)
- [type stubs](type-stubs.md)
- [Getting started](../../getting-started/overview.md)
- [globals](globals.md)
- [LuauAPI mod guidelines](../../mod_guidelines.md)
- [ImGui draw scheduler](../../contributor/internals/imgui-draw-scheduler.md)
- [Limits and errors](../cpp/limits-and-errors.md)

## Source

- `tools/luau_codegen/extra_bindings/imgui.dluau`
- `src/bindings/imgui/ImGuiCore.cpp`
- `src/bindings/imgui/ImGuiWidgetsLayout.cpp`
- `src/bindings/imgui/ImGuiPopupsTablesMenus.cpp`
- `src/bindings/imgui/ImGuiStyleFonts.cpp`
- `src/bindings/imgui/ImGuiBindingInternal.hpp`
