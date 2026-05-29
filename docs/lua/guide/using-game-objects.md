# Using game objects

## Summary

This page shows how to read and create game objects from a script. It covers the `geode.cocos2d` and `geode.gd` namespaces, table shaped values such as points and sizes, and per object fields.

## The two namespaces

- `geode.cocos2d` holds engine classes, for example `CCDirector`, `CCLabelBMFont`, `CCScale9Sprite`, and action classes such as `CCMoveTo`.
- `geode.gd` holds Geometry Dash classes, for example `GameManager`, `GameStatsManager`, and `MenuLayer`.

You usually begin from a singleton or a factory.

```lua
local cocos2d = geode.cocos2d
local gd = geode.gd

local director = cocos2d.CCDirector.sharedDirector()
local gm = gd.GameManager.get()
```

## Creating objects

Many classes provide a `create` factory. Always check the result, because it can be nil.

```lua
local label = cocos2d.CCLabelBMFont.create("Hello, World!", "bigFont.fnt")
if not label then return end
label:setScale(0.5)
```

## Calling methods and reading properties

Call methods with a colon. Read and write simple properties with a dot.

```lua
local opens = self.m_fields.statsOpens
local pos = self.m_obPosition
```

## Table shaped values

Some method arguments are small structs that you pass as plain Lua tables with named keys.

- Point: `{ x = number, y = number }`
- Size: `{ width = number, height = number }`
- Color: `{ r = number, g = number, b = number }`
- Rect: `{ origin = point, size = size }`

```lua
label:setPosition({ x = 100, y = 200 })
bg:setContentSize({ width = 170, height = 84 })
```

## Per object fields

You often want to store your own data on a game object. Use `geode.fields(node)` or the `m_fields` property. Both return the same plain table for that object, and the table lives as long as the object does.

```lua
local fields = geode.fields(self)
fields.count = (fields.count or 0) + 1
```

## Ownership

The runtime tracks whether an object is owned by Lua or only borrowed, and it handles this for you. You do not manage retain or release by hand. For the details, see the framework page below.

## Related

- [Hooks](hooks.md)
- [Type stubs and editor setup](../reference/type-stubs.md)
- [Bindings framework](../../dev/bindings-framework.md)

## Source

- `src/lua/bindings/framework/Types.hpp`
- `src/lua/bindings/framework/Fields.cpp`
- `tools/luau_codegen/hooks.py`
- `tools/luau_codegen/emit_luau_types.py`
