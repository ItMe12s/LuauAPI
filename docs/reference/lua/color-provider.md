# ColorProvider

## Summary

`geode.ColorProvider` is a shared index of named colors.
Mods define colors by id, and texture packs can then change them.
Use it for colors you want a texture pack to override.
Prefix the id with your mod id.

Colors use plain tables. `RGBAColor` is `{ r, g, b, a }` and `RGBColor` is `{ r, g, b }`.
Each field is 0 to 255. An unknown id reads as white.

## define

```lua
geode.ColorProvider.define(id: string, color: RGBAColor) -> RGBAColor
```

Defines a color for an id. Does nothing if the id is already defined. Returns the current color.

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

## color and color3b

```lua
geode.ColorProvider.color(id: string) -> RGBAColor
geode.ColorProvider.color3b(id: string) -> RGBColor
```

`color` returns the current color for an id. `color3b` is the same but drops the alpha.

## Example

```lua
local modId = geode.Mod.getID()
local id = modId .. "/accent"

geode.ColorProvider.define(id, { r = 255, g = 128, b = 0, a = 255 })

local c = geode.ColorProvider.color(id)
print(c.r, c.g, c.b) -- 255 128 0
```

## Related

- [Globals](globals.md)
- [cocos](cocos.md)
- [mod](mod.md)

## Source

- `src/bindings/geode/GeodeSmallBindings.cpp`
