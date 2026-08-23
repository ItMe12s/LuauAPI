# UI and layouts

## Summary

How to build interface elements from a script using the Geode UI classes exposed under `geode.*`.
Covers the factory pattern, the layout system, popup queues, and scene helpers.
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

local sprite = geode.CircleButtonSprite.create(
    cc2d.CCSprite.create("GJ_plusBtn_001.png"),
    geode.CircleBaseSize.Medium,
    geode.CircleBaseColor.Blue
)
if not sprite then return end
sprite:setID(modId .. "/add-button")
```

Generated bindings require every argument listed in the stub, even where C++ has defaults.

Common factories include:

- Button sprites: `CircleButtonSprite`, `IconButtonSprite`
- Text: `TextInput`, `TextRenderer`, `SimpleTextArea`, `MDTextArea`
- Lists: `ListView`, `ListBorders`
- Overlays: `Notification`, `LoadingSpinner`, `ProgressBar`, `ColorPickPopup`

## Layouts

A layout positions a node's children for you.
Set a layout on any `CCNode` with `:setLayout(layout, apply, respectAnchor)`,
then call `:updateLayout(updateChildOrder)` after adding children.
Both calls require every argument. The flag semantics come from the Geode SDK.

- `geode.AxisLayout`, with shorthands `geode.RowLayout` and `geode.ColumnLayout`, arranges children along an axis.
- `geode.AnchorLayout` pins children to anchor points of the parent.
- The `*Options` factories tweak per-child behavior, attached with `:setLayoutOptions()`.

```lua
local cc2d = geode.cocos2d
local modId = geode.Mod.getID()

local menu = cc2d.CCMenu.create()
menu:setID(modId .. "/my-menu")
menu:setLayout(geode.ColumnLayout.create(), true, false)
-- Add your buttons to the menu here.
menu:updateLayout(true)
```

## Quick popups

`geode.createQuickPopup` is a free function that builds a modal alert with up to two buttons and a callback.
The callback receives the popup and which button was pressed.

```lua
geode.createQuickPopup(
    "Confirm",            -- Popup title.
    "Delete this level?", -- Popup body.
    "Cancel", "Delete",   -- First and second buttons.
    function(_popup, btn2)
        if btn2 then print("deleted") end
    end,
    true, -- Parameter doShow: Show the popup now.
    false -- Parameter cancelledByEscape: Escape does not close it.
)
```

## Popup queue

`geode.PopupManager` serializes modal popups so they do not overlap.
Use `alert` for a one-button message, `quickPopup` for a two-button prompt, or `manage` to queue an existing popup.
`geode.PopupManager.DEFAULT_WIDTH` is the standard alert width.

`manage` returns a `ManagedPopup`:

| Method                 | Role                                            |
| ---------------------- | ----------------------------------------------- |
| `getInner`             | Get the managed native popup                    |
| `setPersistent`        | Keep the popup visible across scene transitions |
| `setPriority`          | Set its queue priority                          |
| `blockClosingFor`      | Temporarily prevent closing                     |
| `showInstant`          | Show immediately                                |
| `showQueue`            | Add it to the popup queue                       |
| `isShown`              | Whether it is currently shown                   |
| `shouldPreventClosing` | Whether closing is currently blocked            |

Use `geode.PopupManager.isManaged(popup)` to test a popup and
`geode.PopupManager.hasPendingPopups()` to test the queue.
Popup callbacks follow [Limits and errors](../cpp/limits-and-errors.md).

## Other geode helpers

The stub also exposes a few free functions on `geode` itself:

- `geode.createDefaultLogo()` and `geode.createServerModLogo(modId)` for mod branding nodes.
  `createServerModLogo` takes a server mod ID string, not a local package path.
- `geode.openModsList()` to open the in-game mod list.
- `geode.openInfoPopup(modID)` to show an installed mod or fetch mod info.
  It returns a `GeodeTaskHandle<boolean>?`. It can be nil when no async lookup is needed.
  For the exact condition, see the [Geode SDK docs](https://docs.geode-sdk.org/).
- `geode.Notification.create(text, icon, duration)` for toast overlays.
  Pass an icon from `geode.NotificationIcon.*`.
- `geode.pushSceneWithLayer(layer)` to wrap a `CCLayer` in a new scene and push it.

A toast notification:

```lua
local note = geode.Notification.create("Saved!", geode.NotificationIcon.Success, 2)
if note then
    note:show()
end
```

Queueing an existing popup:

```lua
local popup = geode.PopupManager.manage(alertLayer)
if popup then
    popup:blockClosingFor(2)
    popup:showQueue()
end
```

A `Button` node's `:setDisplayNode(node)` replaces its display node while preserving Geode's button Z-order behavior.

Not every factory is listed here.
For the rest, read [type stubs](type-stubs.md) and the [Geode SDK docs](https://docs.geode-sdk.org/).

## Related

- [globals](globals.md)
- [game objects](game-objects.md)
- [imgui](imgui.md)
- [callbacks](callbacks.md)
- [type stubs](type-stubs.md)
- [gd3d](gd3d.md)
- [LuauAPI mod guidelines](../../mod_guidelines.md)

## Source

- `tools/luau_codegen/emit/luau_types/factories.py`
- `tools/luau_codegen/parse/geode_sdk.py`
- `tools/luau_codegen/model/free_fn_sources.py`
- `tools/luau_codegen/extra_bindings/popup.dluau`
- `src/bindings/geode/GeodePopupBinding.cpp`
- `types/geode.d.luau`
