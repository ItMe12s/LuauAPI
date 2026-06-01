# UI and layouts

## Summary

This page shows how to build interface elements from a script using the Geode UI classes exposed under `geode.*`.
It covers the factory pattern, the layout system, and quick popups.
For signatures, use editor autocomplete. For class details, see the [Geode docs](https://docs.geode-sdk.org/).

## The factory pattern

Geode's UI classes are exposed directly under the `geode` namespace, alongside `cocos2d` and `gd`.
You create one through its factory `create`, the same way you create engine objects. The result can be nil, so check it.

```lua
local sprite = geode.CircleButtonSprite.create(geode.cocos2d.CCSprite.create("GJ_plusBtn_001.png"))
if not sprite then return end
sprite:setID(geode.Mod.getID() .. "/add-button")
```

Commonly used factories include:

- button sprites (`CircleButtonSprite`, `IconButtonSprite`, `CrossButtonSprite`, `EditorButtonSprite`, `TabButtonSprite`),
- text (`TextInput`, `TextRenderer`, `SimpleTextArea`, `MDTextArea`), lists (`ListView`, `ListBorders`),
- and overlay / other useful stuff (`Notification`, `LoadingSpinner`, `ProgressBar`, `ColorPickPopup`).

Every node you create must get a mod-prefixed string ID.
See [Using game objects](using-game-objects.md#node-ids-mod-compatibility).

## Layouts

A layout positions a node's children for you, so you do not place each child by hand.
You set a layout on any `CCNode` with `:setLayout()`, then call `:updateLayout()` after adding children.

- `geode.AxisLayout`, and the shorthands `geode.RowLayout` / `geode.ColumnLayout`, arrange children along an axis.
- `geode.AnchorLayout` pins children to anchor points of the parent.
- The `*Options` factories (`AxisLayoutOptions`, `AnchorLayoutOptions`, ...) tweak per-child behavior, attached with `:setLayoutOptions()`.

```lua
local menu = geode.cocos2d.CCMenu.create()
menu:setID(geode.Mod.getID() .. "/my-menu")
menu:setLayout(geode.ColumnLayout.create())
-- add your buttons to `menu` here
menu:updateLayout()
```

## Quick popups

`geode.createQuickPopup` is a free function (not a factory) that builds a modal alert with up to two buttons and a callback.
The callback receives the popup and which button was pressed.

```lua
geode.createQuickPopup(
    "Confirm",            -- title
    "Delete this level?", -- body
    "Cancel", "Delete",   -- button 1, button 2
    function(_popup, btn2)
        if btn2 then print("deleted") end
    end,
    true, -- pop the scene's existing popups first
    false -- align the buttons to the left
)
```

## Finding signatures

These classes are generated from the Geode bindings.
The authoritative argument lists live in the generated type stubs, surfaced as editor autocomplete.
See [Type stubs and editor setup](../reference/type-stubs.md).
For what a class is for and how the underlying C++ behaves, read the upstream [Geode UI documentation](https://docs.geode-sdk.org/).

## Related

- [Using game objects](using-game-objects.md)
- [Type stubs and editor setup](../reference/type-stubs.md)
- [Geode docs](https://docs.geode-sdk.org/)

## Source

- `tools/luau_codegen/emit/luau_types/factories.py`
- `tools/luau_codegen/parse/geode_sdk.py`
- `types/geode.d.luau`
