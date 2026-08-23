# Keybind

## Summary

`geode.Keybind` reads and writes key combinations. A keybind is a plain table.

| Field       | Type   | Notes                            |
| ----------- | ------ | -------------------------------- |
| `key`       | number | a key code                       |
| `modifiers` | number | a bit mask of held modifier keys |

Use `geode.cocos.enumKeyCodes.KEY_*` values for `key`.
Use `geode.KeyboardModifier` constants for `modifiers`.
`modifiers` is optional and defaults to `0`.

`fromString` returns `nil` and an error message on a bad string.
`toString` and `createNode` raise a Lua error when the `key` field is missing or not a number.
See [globals](globals.md) Error shapes.

## fromString

```lua
geode.Keybind.fromString(str: string) -> ({ key: number, modifiers: number }?, string?)
```

Parses a string such as `"Ctrl+A"` into a keybind.
Modifier and key tokens use `+` with no spaces.
Returns the keybind, or `nil` and an error message.

## toString

```lua
geode.Keybind.toString(keybind: { key: number, modifiers: number }) -> string
```

Turns a keybind back into a readable string.

## createNode

```lua
geode.Keybind.createNode(keybind: { key: number, modifiers: number }) -> CCNode?
```

Builds a node that shows the keybind. Returns the node, or `nil` if it could not be made.

## Example

```lua
local kb, err = geode.Keybind.fromString("Ctrl+A")
if not kb then return print(err) end

print(geode.Keybind.toString(kb)) -- Ctrl+A

local node = geode.Keybind.createNode(kb)
if not node then return end

local menu = self:getChildByID("bottom-menu")
if menu then menu:addChild(node) end
```

Test held modifiers with `bit32.band`:

```lua
local mods = geode.KeyboardModifier

local function hasControl(kb)
    return bit32.band(kb.modifiers, mods.Control) ~= 0
end
```

## Related

- [globals](globals.md)
- [Keyboard input](keyboard-input.md)
- [delegates](delegates.md)
- [UI and layouts](ui.md)

## Source

- `src/bindings/geode/GeodeSmallBindings.cpp`
