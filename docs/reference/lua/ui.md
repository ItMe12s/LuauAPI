# UI and layouts

## Summary

How to build interface elements from a script using the Geode UI classes exposed under `geode.*`.
Covers the factory pattern, the layout system, and quick popups. For signatures, use editor
autocomplete. For class behavior, see the [Geode docs](https://docs.geode-sdk.org/).

## The factory pattern

Geode UI classes sit directly under `geode`, next to `cocos2d` and `gd`. You create one through its
`create` factory, the same way you create engine objects. The result can be nil, so check it.

```lua
local cc2d = geode.cocos2d
local modId = geode.Mod.getID()

local sprite = geode.CircleButtonSprite.create(cc2d.CCSprite.create("GJ_plusBtn_001.png"))
if not sprite then return end
sprite:setID(modId .. "/add-button")
```

Common factories include button sprites (`CircleButtonSprite`, `IconButtonSprite`), text
(`TextInput`, `TextRenderer`, `SimpleTextArea`, `MDTextArea`), lists (`ListView`, `ListBorders`),
and overlays (`Notification`, `LoadingSpinner`, `ProgressBar`, `ColorPickPopup`). Every node you
create must get a mod-prefixed string ID. See [Game objects](game-objects.md).

## Layouts

A layout positions a node's children for you. Set a layout on any `CCNode` with `:setLayout()`, then
call `:updateLayout()` after adding children.

- `geode.AxisLayout`, with shorthands `geode.RowLayout` and `geode.ColumnLayout`, arranges children
  along an axis.
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

`geode.createQuickPopup` is a free function that builds a modal alert with up to two buttons and a
callback. The callback receives the popup and which button was pressed.

```lua
geode.createQuickPopup(
    "Confirm",            -- title
    "Delete this level?", -- body
    "Cancel", "Delete",   -- button 1, button 2
    function(_popup, btn2)
        if btn2 then print("deleted") end
    end,
    true, -- pop existing popups first
    false -- align buttons to the left
)
```

## Finding signatures

These classes are generated from the Geode bindings. The authoritative argument lists live in the
generated type stubs, surfaced as editor autocomplete. See [Type stubs](type-stubs.md). For class
behavior, read the upstream [Geode UI docs](https://docs.geode-sdk.org/).

## Related

- [Game objects](game-objects.md)
- [Type stubs](type-stubs.md)

## Source

- `tools/luau_codegen/emit/luau_types/factories.py`
- `tools/luau_codegen/parse/geode_sdk.py`
- `types/geode.d.luau`
