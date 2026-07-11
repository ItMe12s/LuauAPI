# cocos

## Summary

`geode.cocos` holds helper functions for cocos2d nodes and colors.
These are utility functions, not classes. The node classes themselves live on `geode.cocos2d`.

`geode.cocos` is a hybrid namespace. Node helpers and hex string functions come from codegen over `Geode/utils/cocos.hpp`.
Color helpers and hex parsers are handwritten in `GeodeCocosBinding.cpp`.

Colors use plain tables:

| Type | Shape | Notes |
| --- | --- | --- |
| `RGBColor` | `{ r, g, b }` | each field 0 to 255 |
| `RGBAColor` | `{ r, g, b, a }` | each field 0 to 255 |
| `RGBAFloatColor` | `{ r, g, b, a }` | each field 0.0 to 1.0 |
| `HSVColor` | `{ h, s, v }` | hue, saturation, and value |

`geode.Color` provides Geode's color conversions and adjustments while keeping these plain table shapes.

## Node functions

| Function | Description |
| --- | --- |
| `calculateChildCoverage(node: CCNode) -> CCRect` | Bounding rect of a node's children. |
| `calculateNodeCoverage(nodes: CCArray) -> CCRect` | Bounding rect of an array of nodes. |
| `switchToScene(layer: CCLayer) -> CCScene?` | Wrap a layer in a scene and switch to it. |
| `nodeIsVisible(node: CCNode) -> boolean` | True if the node and all parents are visible. |
| `getChildByTagRecursive(node: CCNode, tag: number) -> CCNode?` | Find a child by tag at any level. |
| `isSpriteFrameName(node: CCNode, name: string) -> boolean` | True if the node uses that sprite frame. |
| `getChildBySpriteFrameName(node: CCNode, name: string) -> CCNode?` | First child using that sprite frame. |
| `isSpriteName(node: CCNode, name: string) -> boolean` | True if the node uses that sprite. |
| `getChildBySpriteName(node: CCNode, name: string) -> CCNode?` | First child using that sprite. |
| `getMousePos() -> CCPoint` | Mouse position in cocos coordinates. |
| `getLabelSize(text: string, font: string, kerning: number) -> CCSize` | Size of text in a bitmap font. |
| `fileExistsInSearchPaths(filename: string) -> boolean` | True if the file is in the search paths. |
| `limitNodeSize(node: CCNode, size: CCSize, default: number, min: number) -> ()` | Scale a node to fit a size. |
| `limitNodeWidth(node: CCNode, width: number, default: number, min: number) -> ()` | Scale to fit a width. |
| `limitNodeHeight(node: CCNode, height: number, default: number, min: number) -> ()` | Scale to fit a height. |

## Color functions

### geode.Color

| Function | Description |
| --- | --- |
| `parse(hex) -> (RGBAColor?, string?)` | Parse a hex color with a recoverable error result. |
| `fromHSV(hsv, alpha?) -> RGBAColor` | Convert HSV to RGBA. |
| `toHSV(color) -> HSVColor` | Convert RGBA to HSV. |
| `to3B`, `to4B`, `to4F` | Convert between supported color table shapes. |
| `applyHSV(color, hsv) -> RGBAColor` | Apply an HSV adjustment without mutating the input. |
| `isInvisible`, `isTranslucent`, `isOpaque` | Test alpha state. |

Geode's H/S/V/B adjustment aliases are also exposed on `geode.Color` and return a new color.
Use editor autocomplete for their exact names and signatures.

### geode.cocos

| Function | Description |
| --- | --- |
| `invert3B(color: RGBColor) -> RGBColor` | Invert red, green, and blue. |
| `invert4B(color: RGBAColor) -> RGBAColor` | Invert the color, keep the alpha. |
| `lighten3B(color: RGBColor, amount: number) -> RGBColor` | Add `amount` to each channel. |
| `darken3B(color: RGBColor, amount: number) -> RGBColor` | Subtract `amount` from each channel. |
| `to3B(color: RGBAColor) -> RGBColor` | Drop the alpha. |
| `to4B(color: RGBColor, alpha: number?) -> RGBAColor` | Add an alpha, default 255. |
| `to4F(color: RGBAColor) -> RGBAFloatColor` | Convert 0 to 255 values to 0.0 to 1.0. |
| `cc3bToHexString(color: RGBColor) -> string` | Color as uppercase `RRGGBB`. |
| `cc4bToHexString(color: RGBAColor) -> string` | Color as uppercase `RRGGBBAA`. |
| `cc3bFromHexString(hex: string, permissive: boolean?) -> (RGBColor?, string?)` | Parse a hex string. |
| `cc4bFromHexString(hex: string, requireAlpha: boolean?, permissive: boolean?) -> (RGBAColor?, string?)` | Parse a hex string with alpha. |
| `ccDrawColor4B(color: RGBAColor) -> ()` | Set the draw color, only valid inside a draw call. |

The hex parse functions use the recoverable error shape. See [globals](globals.md) Error shapes.
With `permissive` true, a short string like `"f"` reads as white.

## enumKeyCodes

```lua
geode.cocos.enumKeyCodes.KEY_A
geode.cocos.enumKeyCodes.KEY_Escape
geode.cocos.enumKeyCodes.MOUSE_4
geode.cocos.enumKeyCodes.CONTROLLER2_A
geode.cocos.enumKeyCodes.CONTROLLER_L3
geode.cocos.enumKeyCodes.CONTROLLER2_R3
```

`geode.cocos.enumKeyCodes` contains the full cocos `enumKeyCodes` table.
Use these values with keyboard delegates, [Keyboard input](keyboard-input.md), and [Keybind](keybind.md).
See [enums](enums.md) for how enum values compare at runtime.

## Example

```lua
local cc = geode.cocos

local red, err = cc.cc3bFromHexString("ff0000")
if not red then return print(err) end

print(cc.cc3bToHexString(red)) -- FF0000
print(cc.lighten3B(red, 20).r) -- 255
```

## Related

- [globals](globals.md)
- [enums](enums.md)
- [game objects](game-objects.md)
- [ColorProvider](color-provider.md)
- [Keyboard input](keyboard-input.md)
- [gd3d](gd3d.md)

## Source

- `tools/luau_codegen/model/free_fn_sources.py`
- `src/bindings/geode/GeodeCocosBinding.cpp`
- `src/bindings/geode/GeodeSmallBindings.cpp`
- `tools/luau_codegen/extra_bindings/color.dluau`
- `tools/luau_codegen/emit/luau_types/manual_fields.py`
- `build/luauapi-gen/src/`
- `types/geode.d.luau`
