# Keybind

## Summary

`geode.Keybind` reads and writes key combinations. A keybind is a plain table.

| Field | Type | Notes |
| --- | --- | --- |
| `key` | number | a key code |
| `modifiers` | number | a bit mask of held modifier keys |

`fromString` returns `nil` and an error message on a bad string, so you can handle it without `pcall`.

## fromString

```lua
geode.Keybind.fromString(str: string) -> ({ key: number, modifiers: number }?, string?)
```

Parses a string such as `"Ctrl + A"` into a keybind.
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
local kb, err = geode.Keybind.fromString("Ctrl + A")
if not kb then return print(err) end

print(geode.Keybind.toString(kb)) -- Ctrl + A

local node = geode.Keybind.createNode(kb)
if node then someMenu:addChild(node) end
```

## Related

- [Game objects](game-objects.md)
- [UI and layouts](ui.md)

## Source

- `src/lua/bindings/geode/GeodeKeybindBinding.cpp`
