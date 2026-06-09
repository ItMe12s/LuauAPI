# game objects

## Summary

How to read and create game objects from a script. Covers the `geode.cocos2d` and `geode.gd`
namespaces, table-shaped values like points and sizes, and per-object fields.

## The two namespaces

- `geode.cocos2d` holds engine classes, for example `CCDirector`, `CCLabelBMFont`, `CCSprite`, and
  action classes such as `CCMoveTo`.
- `geode.gd` holds Geometry Dash classes, for example `GameManager`, `GameStatsManager`, `MenuLayer`.

You usually begin from a singleton or a factory.

```lua
local cc2d = geode.cocos2d
local gd = geode.gd

local director = cc2d.CCDirector.sharedDirector()
local gm = gd.GameManager.get()
```

## Creating objects

Many classes provide a `create` factory. Always check the result, because it can be nil.

```lua
local label = cc2d.CCLabelBMFont.create("Hello, World!", "bigFont.fnt")
if not label then return end
label:setScale(0.5)
```

## Node IDs for mod compatibility

Geode adds string IDs to `CCNode`. When you create a node, call `:setID()` with your mod id as a
prefix, lowercase kebab-case, no spaces. String IDs let you find nodes with `getChildByID` and
layouts instead of fragile child indexes, which is how Geode mods stay compatible.

```lua
local cc2d = geode.cocos2d
local modId = geode.Mod.getID()

local sprite = cc2d.CCSprite.createWithSpriteFrameName(modId .. "/coolSprite.png")
if not sprite then return end
sprite:setID(modId .. "/cool-sprite")
```

See the [Geode nodetree tutorial](https://docs.geode-sdk.org/tutorials/nodetree/).

## Calling methods and reading properties

Call methods with a colon. Read and write simple properties with a dot.

```lua
local pos = self.m_obPosition
```

## Table-shaped values

Some method arguments are small structs you pass as plain Lua tables with named keys.

- Point: `{ x = number, y = number }`
- Size: `{ width = number, height = number }`
- Color: `{ r = number, g = number, b = number }`
- Rect: `{ origin = point, size = size }`

```lua
label:setPosition({ x = 100, y = 200 })
bg:setContentSize({ width = 170, height = 84 })
```

## Per-object fields

Store your own data on an object with `geode.fields(node)` or the `m_fields` property. Both return
the same plain table for that object, alive as long as the object.

```lua
local fields = geode.fields(self)
fields.count = (fields.count or 0) + 1
```

## Ownership

The runtime tracks whether an object is owned by Lua or only borrowed and handles it for you. You do
not manage retain or release by hand. See the [bindings framework](../../contributor/internals/bindings-framework.md).

## Related

- [Hooks](hooks.md)
- [UI and layouts](ui.md)
- [Type stubs](type-stubs.md)

## Source

- `src/lua/bindings/framework/Types.hpp`
- `src/lua/bindings/framework/Fields.cpp`
- `tools/luau_codegen/emit/luau_types/`
