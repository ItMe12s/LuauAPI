# UI and layouts

## Summary

How to build interface elements from a script using the Geode UI classes exposed under `geode.*`.
Covers the factory pattern, the layout system, and quick popups.
Signatures come from [type stubs](type-stubs.md). For class behavior, see the [Geode docs](https://docs.geode-sdk.org/).

## Which UI to use

Use [imgui](imgui.md) for player mod menus you can rebuild every frame.
Settings, toggles, tabs, lists, color edits, and confirm popups fit well there.

Use `geode.*` Cocos UI when you need native game nodes, sprites, persistent layers, or layout objects.
Cocos UI lives in the scene graph. ImGui is immediate mode and redraws each frame.

## The factory pattern

Geode UI classes sit directly under `geode`, next to `cocos2d` and `gd`.
You create one through its `create` factory, the same way you create engine objects.
The result can be nil, so check it.

```lua
local cc2d = geode.cocos2d
local modId = geode.Mod.getID()

local sprite = geode.CircleButtonSprite.create(cc2d.CCSprite.create("GJ_plusBtn_001.png"))
if not sprite then return end
sprite:setID(modId .. "/add-button")
```

Common factories include:

- Button sprites: `CircleButtonSprite`, `IconButtonSprite`
- Text: `TextInput`, `TextRenderer`, `SimpleTextArea`, `MDTextArea`
- Lists: `ListView`, `ListBorders`
- Overlays: `Notification`, `LoadingSpinner`, `ProgressBar`, `ColorPickPopup`

## Layouts

A layout positions a node's children for you.
Set a layout on any `CCNode` with `:setLayout()`, then call `:updateLayout()` after adding children.

- `geode.AxisLayout`, with shorthands `geode.RowLayout` and `geode.ColumnLayout`,
  arranges children along an axis.
- `geode.AnchorLayout` pins children to anchor points of the parent.
- The `*Options` factories tweak per-child behavior, attached with `:setLayoutOptions()`.

```lua
local cc2d = geode.cocos2d
local modId = geode.Mod.getID()

local menu = cc2d.CCMenu.create()
menu:setID(modId .. "/my-menu")
menu:setLayout(geode.ColumnLayout.create())
-- add your buttons to menu here
menu:updateLayout()
```

## Quick popups

`geode.createQuickPopup` is a free function that builds a modal alert with up to two buttons and a callback.
The callback receives the popup and which button was pressed.

```lua
geode.createQuickPopup(
    "Confirm",            -- title
    "Delete this level?", -- body
    "Cancel", "Delete",   -- button 1, button 2
    function(_popup, btn2)
        if btn2 then print("deleted") end
    end,
    true, -- doShow: show the popup now
    false -- cancelledByEscape: Escape does not close it
)
```

## Other geode helpers

The stub also exposes a few free functions on `geode` itself:

- `geode.createDefaultLogo()` and `geode.createServerModLogo(modId)` for mod branding nodes.
  `createServerModLogo` takes a server mod ID string, not a local package path.
- `geode.openModsList()` to open the in-game mod list
- `geode.openInfoPopup(modID)` to show an installed mod or fetch mod info.
  It returns nil when the mod is installed.
  Otherwise it returns a `GeodeTaskHandle<boolean>` for the async lookup.
- `geode.Notification.create(text, icon, duration)` for toast overlays

Not every factory is listed here.
For the rest, read [type stubs](type-stubs.md) and the [Geode SDK docs](https://docs.geode-sdk.org/).

## ImGui mod menus

See [imgui](imgui.md) and [mod/demo/demo_modmenu.luau](../../../mod/demo/demo_modmenu.luau) for overlay menus.

## Finding signatures

These classes are generated from the Geode bindings.
The authoritative argument lists live in the generated type stubs, surfaced as editor autocomplete.
See [type stubs](type-stubs.md). For class behavior, read the upstream [Geode UI docs](https://docs.geode-sdk.org/).

## Related

- [Getting started](../../getting-started/overview.md)
- [Examples](../../getting-started/examples.md)
- [LuauAPI mod guidelines](../../mod_guidelines.md)
- [game objects](game-objects.md)
- [callbacks](callbacks.md)
- [imgui](imgui.md)
- [type stubs](type-stubs.md)
- [gd3d](gd3d.md)

## Source

- `tools/luau_codegen/emit/luau_types/factories.py`
- `tools/luau_codegen/parse/geode_sdk.py`
- Generated `types/geode.d.luau` at build time
