# imgui

## Summary

`imgui` draws Dear ImGui UI over the game through gd-imgui-cocos.
Use it for player mod menus, settings panels, tabs, popups, and debug tools.
Types match `tools/luau_codegen/extra_bindings/imgui.dluau`.

## Model

Register one draw callback with `imgui.onDraw`. Build all ImGui UI inside that callback.
Widget calls must run on the main thread and inside `imgui.onDraw`.

ImGui is immediate mode. It does not store your widget state.
Pass current values each frame and save returned values in Luau.

The game keeps input when ImGui is not hovered or focused.
Focused ImGui windows capture the input they need.

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

See [Examples](../../getting-started/examples.md) and [src/scripts/_modmenudemo.luau](../../../src/scripts/_modmenudemo.luau).

## Lifecycle

```lua
imgui.onDraw(fn) -> ImGuiDrawHandle
imgui.cancel(handle)
imgui.setVisible(visible)
imgui.toggle()
imgui.isVisible() -> boolean
imgui.isItemHovered(flags: number?) -> boolean
imgui.isItemClicked(mouseButton: number?) -> boolean
imgui.isWindowHovered(flags: number?) -> boolean
imgui.setNextWindowFocus() -> ()
imgui.setWindowFocus(name: string?) -> ()
```

`onDraw` starts the backend on first use. `cancel` removes a callback.
Dropping the handle cancels it on GC. A callback that errors is removed.

## Windows and layout

```lua
imgui.window(title, fn, opts?) -> boolean
imgui.child(id, fn, opts?)
imgui.group(fn)
imgui.sameLine(offset?, spacing?)
imgui.separator()
imgui.separatorText(label)
imgui.spacing()
imgui.indent(width?)
imgui.unindent(width?)
imgui.dummy(size)
imgui.newLine()
imgui.getContentRegionAvail() -> ImGuiVec2
```

Scoped wrappers always close their ImGui region after `fn`. This also happens when `fn` errors.
`window` returns false when its close button is pressed. You don't have to name the window with your mod name or ID.
Use `sizeCond` or `posCond` with `imgui.Cond.Always` for animated windows.

## Widgets

```lua
imgui.text(text)
imgui.textWrapped(text)
imgui.bulletText(text)
imgui.button(label, opts?) -> boolean
imgui.checkbox(label, value) -> boolean
imgui.checkboxFlags(label, flags, flagValue) -> number
imgui.radioButton(label, active) -> boolean
imgui.sliderFloat(label, value, min, max, fmt?) -> number
imgui.sliderInt(label, value, min, max) -> number
imgui.dragFloat(label, value, opts?) -> number
imgui.dragInt(label, value, opts?) -> number
imgui.inputText(label, value, maxLen?) -> string
imgui.inputTextMultiline(label, value, size?, maxLen?) -> string
imgui.inputFloat(label, value, opts?) -> number
imgui.inputInt(label, value, opts?) -> number
imgui.inputDouble(label, value, opts?) -> number
imgui.progressBar(fraction, opts?)
```

Text is drawn as raw text, so percent signs are safe.
Input text uses a shared per-thread buffer.
Default max length is 16384 and the cap is 65536.

## Lists and colors

```lua
imgui.combo(label, index, items, opts?) -> number
imgui.comboPopup(label, preview, fn, opts?) -> boolean
imgui.selectable(label, selected, opts?) -> boolean
imgui.listBox(label, index, items, opts?) -> number
imgui.colorEdit3(label, color, opts?) -> ImGuiVec3
imgui.colorEdit4(label, color, opts?) -> ImGuiVec4
imgui.colorButton(label, color, opts?) -> boolean
```

Indexes are zero-based. Colors use `{ x, y, z, w }` floats from 0 to 1.
`comboPopup` is scoped and lets you build custom combo contents.

## Trees, Tabs, Popups

```lua
imgui.collapsingHeader(label, opts?) -> boolean, boolean?
imgui.treeNode(label, fn, opts?) -> boolean?
imgui.tabBar(id, fn, opts?)
imgui.tabItem(label, fn, opts?) -> boolean?
imgui.openPopup(id, flags?)
imgui.closeCurrentPopup()
imgui.popup(id, fn, opts?) -> boolean
imgui.popupModal(title, fn, opts?) -> boolean?
imgui.setTooltip(text)
imgui.tooltip(fn)
```

`treeNode` returns a tracked open value only when `opts.open` is set.
`tabItem` and `popupModal` return a close state only when `closable` is true.
Use `openPopup` before `popup` or `popupModal`.
`imgui.tooltip(fn)` shows a tooltip for the previous item only when it is hovered.
Use `imgui.isItemHovered()` before `imgui.setTooltip(text)` when you need manual control.

## Tables And Menus

```lua
imgui.table(id, columns, fn, opts?)
imgui.tableSetupColumn(label, opts?)
imgui.tableHeadersRow()
imgui.tableNextRow(flags?)
imgui.tableNextColumn() -> boolean
imgui.tableSetColumnIndex(column) -> boolean
imgui.columns(count?, opts?)
imgui.nextColumn()
imgui.menuBar(fn)
imgui.menu(label, fn, opts?)
imgui.menuItem(label, selected?, opts?) -> boolean
```

Call table row and column helpers inside `imgui.table`.
`menuItem` returns clicked when no selected value is passed.
When selected is passed, it returns the new selected value.

## Style And Theme

```lua
imgui.theme.apply("dark" | "light" | "classic")
imgui.theme.applyCustom(opts)
imgui.style.with(opts, fn)
```

`imgui.theme.apply` and `imgui.theme.applyCustom` change global ImGui style until changed again.
Use `imgui.style.with` for per-window or per-mod styling that does not affect other `onDraw` callbacks.
`style.with` is scoped to `fn` and pops colors and style vars even when `fn` errors.

Useful option fields include `alpha`, `windowPadding`, `windowRounding`, `framePadding`, `frameRounding`, `itemSpacing`, and `colors`.
Color keys can be `imgui.Col.*` values or color names from `imgui.Col`.

## Flags

Use constants instead of magic numbers:

```lua
imgui.Flag.Window.NoResize
imgui.Flag.Combo.HeightLarge
imgui.Flag.Table.Borders
imgui.Flag.Selectable.SpanAllColumns
imgui.Col.Text
imgui.StyleVar.Alpha
imgui.Cond.FirstUseEver
```

See `imgui.dluau` for all constant tables.

## Limits

Use one `imgui.onDraw` per mod when possible. Keep draw work under 20 ms.
Do not do IO, web work, or heavy list rebuilds in `onDraw`.
Image, texture, font, docking, viewport, and draw list APIs are not bound.
See [Limits and errors](../cpp/limits-and-errors.md) for hard caps and error strings.

## ImGui or Cocos UI

See [UI and layouts](ui.md) for when to use ImGui vs Cocos UI.

## Related

- [UI and layouts](ui.md)
- [Examples](../../getting-started/examples.md)
- [LuauAPI mod guidelines](../../mod_guidelines.md)
- [ImGui draw scheduler](../../contributor/internals/imgui-draw-scheduler.md)
- [Limits and errors](../cpp/limits-and-errors.md)
- [Type stubs](type-stubs.md)

## Source

- `tools/luau_codegen/extra_bindings/imgui.dluau`
- `src/bindings/imgui/ImGuiBinding.cpp`
- `src/bindings/imgui/ImGuiBindingInternal.hpp`
- `src/bindings/imgui/ImGuiWidgets.cpp`
- `src/bindings/imgui/ImGuiLayout.cpp`
- `src/bindings/imgui/ImGuiPopups.cpp`
- `src/bindings/imgui/ImGuiTables.cpp`
- `src/bindings/imgui/ImGuiMenus.cpp`
- `src/bindings/imgui/ImGuiStyle.cpp`
- `src/bindings/imgui/ImGuiConstants.cpp`
