# Reference: cocos

## Summary

This page lists `geode.cocos`. It holds helper functions for cocos2d nodes and colors.

These are utility functions, not classes. The node classes themselves live on
`geode.cocos2d`. See [Lua overview](../overview.md).

Colors use plain tables:

| Type | Shape | Notes |
| --- | --- | --- |
| `RGBColor` | `{ r, g, b }` | each field 0 to 255 |
| `RGBAColor` | `{ r, g, b, a }` | each field 0 to 255 |
| `RGBAFloatColor` | `{ r, g, b, a }` | each field 0.0 to 1.0 |

## Node functions

| Function | Description |
| --- | --- |
| `calculateChildCoverage(node: CCNode) -> CCRect` | Bounding rect of a node's children. |
| `calculateNodeCoverage(nodes: CCArray) -> CCRect` | Bounding rect of an array of nodes. |
| `switchToScene(layer: CCLayer) -> CCScene?` | Wrap a layer in a scene and switch to it. |
| `nodeIsVisible(node: CCNode) -> boolean` | True if the node and all parents are visible. |
| `getChildByTagRecursive(node: CCNode, tag: number) -> CCNode?` | Find a child by tag, searching all levels. |
| `isSpriteFrameName(node: CCNode, name: string) -> boolean` | True if the node uses that sprite frame. |
| `getChildBySpriteFrameName(node: CCNode, name: string) -> CCNode?` | First child using that sprite frame. |
| `isSpriteName(node: CCNode, name: string) -> boolean` | True if the node uses that sprite. |
| `getChildBySpriteName(node: CCNode, name: string) -> CCNode?` | First child using that sprite. |
| `getObjectName(object: CCObject) -> string` | Class name of an object. |
| `getMousePos() -> CCPoint` | Mouse position in cocos coordinates. |
| `getLabelSize(text: string, font: string, kerning: number) -> CCSize` | Size of text drawn in a bitmap font. |
| `fileExistsInSearchPaths(filename: string) -> boolean` | True if the file is found in the search paths. |
| `limitNodeSize(node: CCNode, size: CCSize, default: number, min: number) -> ()` | Scale a node to fit a size. |
| `limitNodeWidth(node: CCNode, width: number, default: number, min: number) -> ()` | Scale a node to fit a width. |
| `limitNodeHeight(node: CCNode, height: number, default: number, min: number) -> ()` | Scale a node to fit a height. |
| `handleTouchPriority(node: CCNode, force: boolean?) -> ()` | Fix touch priority for a node tree. |
| `handleTouchPriorityWith(node: CCNode, priority: number, force: boolean?) -> ()` | Set touch priority for a node tree. |

## Color functions

| Function | Description |
| --- | --- |
| `invert3B(color: RGBColor) -> RGBColor` | Invert the red, green, and blue values. |
| `invert4B(color: RGBAColor) -> RGBAColor` | Invert the color, keep the alpha. |
| `lighten3B(color: RGBColor, amount: number) -> RGBColor` | Add `amount` to each channel. |
| `darken3B(color: RGBColor, amount: number) -> RGBColor` | Subtract `amount` from each channel. |
| `to3B(color: RGBAColor) -> RGBColor` | Drop the alpha. |
| `to4B(color: RGBColor, alpha: number?) -> RGBAColor` | Add an alpha. Defaults to 255. |
| `to4F(color: RGBAColor) -> RGBAFloatColor` | Convert 0 to 255 values to 0.0 to 1.0. |
| `cc3bToHexString(color: RGBColor) -> string` | Color as `rrggbb`. |
| `cc4bToHexString(color: RGBAColor) -> string` | Color as `rrggbbaa`. |
| `cc3bFromHexString(hex: string, permissive: boolean?) -> (RGBColor?, string?)` | Parse a hex string. Returns the color, or nil and an error. |
| `cc4bFromHexString(hex: string, requireAlpha: boolean?, permissive: boolean?) -> (RGBAColor?, string?)` | Parse a hex string with alpha. Returns the color, or nil and an error. |
| `ccDrawColor4B(color: RGBAColor) -> ()` | Set the draw color. Only valid inside a draw call. |

The hex parse functions return `nil` and an error message on a bad string, so you can
handle them without `pcall`. With `permissive` set to true a short string like `"f"` reads
as white.

## Example

```lua
local cc = geode.cocos

local red, err = cc.cc3bFromHexString("ff0000")
if not red then
    print(err)
    return
end

print(cc.cc3bToHexString(red)) -- ff0000
print(cc.lighten3B(red, 20).r) -- 255

local node = someLayer:getChildByID("my-node")
if cc.nodeIsVisible(node) then
    cc.limitNodeWidth(node, 100, 1.0, 0.2)
end
```

## Related

- [Reference: ColorProvider](color-provider.md)
- [Using game objects](../guide/using-game-objects.md)
- [Lua overview](../overview.md)

## Source

- `src/lua/bindings/geode/GeodeCocosBinding.cpp`
- `tools/luau_codegen/emit/luau_types/manual_fields.py`
