# Keyboard input

## Summary

`geode.KeyboardInputEvent` listens to Geode keyboard input events.
Callbacks receive a mutable `KeyboardInputData` table. Return `true` to stop propagation.
For signatures, use editor autocomplete from [type stubs](type-stubs.md).

Use `geode.cocos.enumKeyCodes.KEY_*` values for keys and `geode.KeyboardModifier` for modifier masks.

## KeyboardInputData

| Field       | Type                              | Notes                              |
| ----------- | --------------------------------- | ---------------------------------- |
| `key`       | number                            | `geode.cocos.enumKeyCodes.KEY_*`   |
| `action`    | number                            | `geode.KeyboardInputData.Action.*` |
| `modifiers` | number                            | bit mask of held modifier keys     |
| `timestamp` | number                            | native event time                  |
| `native`    | `{ code: number, extra: number }` | platform-specific key data         |

## KeyboardModifier

```lua
geode.KeyboardModifier.None: number
geode.KeyboardModifier.Shift: number
geode.KeyboardModifier.Control: number
geode.KeyboardModifier.Alt: number
geode.KeyboardModifier.Super: number
```

Modifier values are bit masks. Combine or test them with Luau bit operations.

## KeyboardInputData.Action

```lua
geode.KeyboardInputData.Action.Press: number
geode.KeyboardInputData.Action.Release: number
geode.KeyboardInputData.Action.Repeat: number
```

## listen

```lua
geode.KeyboardInputEvent.listen(callback: (data: KeyboardInputData) -> boolean?, priority: number?) -> KeyboardInputListenerHandle
```

Listens to every keyboard input event.

## listenFor

```lua
geode.KeyboardInputEvent.listenFor(key: number, callback: (data: KeyboardInputData) -> boolean?, priority: number?) -> KeyboardInputListenerHandle
```

Listens only for one key code.

## disconnect

```lua
handle:disconnect() -> ()
```

Disconnects a listener handle. Handles also disconnect during garbage collection and runtime shutdown.

## Return value

Return `true` from the callback to stop propagation to later listeners.
Return `false` or nothing to let the event continue.
If a callback errors or times out, LuauAPI logs it and lets propagation continue.

## Ctrl + A example

Test a key and its modifiers with `bit32.band`:

```lua
local keys = geode.cocos.enumKeyCodes
local mods = geode.KeyboardModifier
local action = geode.KeyboardInputData.Action

local handle = geode.KeyboardInputEvent.listenFor(keys.KEY_A, function(data)
    if data.action ~= action.Press then return false end
    if bit32.band(data.modifiers, mods.Control) == 0 then return false end

    print("Ctrl+A")
    return true
end, 100)
```

The optional priority argument works like other Geode event listeners.
Higher priority runs first. A listener that returns `true` blocks lower ones.

## Listen to every key example

Use `listen` to filter many keys in one place.
`Repeat` fires while a key is held, so ignore it when you only want one action per physical press.

```lua
local action = geode.KeyboardInputData.Action

geode.KeyboardInputEvent.listen(function(data)
    local act = if data.action == action.Press then "press"
        elseif data.action == action.Release then "release"
        else "repeat"

    print("key", data.key, act, "mods", data.modifiers, "native code", data.native.code)
    return false
end)
```

## Rewrite mutable data example

The callback gets a copy of the event data. Your changes are written back before later listeners run, so use this sparingly.

```lua
local keys = geode.cocos.enumKeyCodes
local action = geode.KeyboardInputData.Action

geode.KeyboardInputEvent.listen(function(data)
    if data.key == keys.KEY_W and data.action == action.Press then
        data.key = keys.KEY_Up
    end
    return false
end)
```

Store the handle and call `handle:disconnect()` when you no longer need input.

## Related

- [Mouse input](mouse-input.md)
- [Getting started](../../getting-started/overview.md)
- [callbacks](callbacks.md)
- [Keybind](keybind.md)
- [cocos](cocos.md)
- [delegates](delegates.md)
- [globals](globals.md)
- [type stubs](type-stubs.md)

## Source

- `src/bindings/geode/GeodeKeyboardBinding.cpp`
- `tools/luau_codegen/extra_bindings/keyboard.dluau`
- `tools/luau_codegen/parse/cocos_enums.py`
