# Reference: ColorProvider

## Summary

This page lists `geode.ColorProvider`. It is a shared index of named colors.

Mods define colors by id. Texture packs can then change those colors. Use it for colors
you want a texture pack to be able to override. Prefix the id with your mod id.

Colors use plain tables. `RGBAColor` is `{ r, g, b, a }` and `RGBColor` is `{ r, g, b }`.
Each field is 0 to 255. An unknown id reads as white.

## define

```lua
geode.ColorProvider.define(id: string, color: RGBAColor) -> RGBAColor
```

Defines a color for an id. Does nothing if the id is already defined. Returns the current
color.

## override

```lua
geode.ColorProvider.override(id: string, color: RGBAColor) -> RGBAColor
```

Overrides the current color for an id. Returns the new color.

## reset

```lua
geode.ColorProvider.reset(id: string) -> RGBAColor
```

Resets a color back to its defined value. Returns that value.

## color

```lua
geode.ColorProvider.color(id: string) -> RGBAColor
```

Returns the current color for an id.

## color3b

```lua
geode.ColorProvider.color3b(id: string) -> RGBColor
```

Same as `color`, but drops the alpha.

## Example

```lua
local modId = geode.Mod.getID()
local id = modId .. "/accent"

geode.ColorProvider.define(id, { r = 255, g = 128, b = 0, a = 255 })

local c = geode.ColorProvider.color(id)
print(c.r, c.g, c.b) -- 255 128 0
```

## Related

- [Reference: cocos](cocos.md)
- [Lua overview](../overview.md)

## Source

- `src/lua/bindings/geode/GeodeColorProviderBinding.cpp`
