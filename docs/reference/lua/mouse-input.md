# Mouse input

## Summary

Use `geode.MouseInputEvent` for mouse buttons.
Use `geode.MouseMoveEvent` for movement and `geode.ScrollWheelEvent` for scrolling.
Return `true` from a callback to stop propagation to later listeners and the game.

## MouseInputData

Mouse button callbacks receive a mutable table.

| Field | Type | Notes |
| --- | --- | --- |
| `button` | number | `geode.MouseInputData.Button.*` |
| `action` | number | `geode.MouseInputData.Action.*` |
| `modifiers` | number | `geode.KeyboardModifier` bit mask |
| `timestamp` | number | native event time |

```lua
geode.MouseInputData.Action.Press: number
geode.MouseInputData.Action.Release: number

geode.MouseInputData.Button.Left: number
geode.MouseInputData.Button.Right: number
geode.MouseInputData.Button.Middle: number
geode.MouseInputData.Button.Button4: number
geode.MouseInputData.Button.Button5: number
```

Changes to the table are written back before later listeners run.

## MouseInputEvent

```lua
geode.MouseInputEvent.listen(callback: (data: MouseInputData) -> boolean?, priority: number?) -> MouseInputListenerHandle
```

Listens to mouse button presses and releases.

```lua
local button = geode.MouseInputData.Button
local action = geode.MouseInputData.Action

local listener = geode.MouseInputEvent.listen(function(data)
    if data.button == button.Left and data.action == action.Press then
        print("left click", data.timestamp)
        return true -- This won't let you click on anything.
    end
    return false
end)
```

## MouseMoveEvent

```lua
geode.MouseMoveEvent.listen(callback: (x: number, y: number) -> boolean?, priority: number?) -> MouseInputListenerHandle
```

Listens to mouse movement in native window coordinates.

```lua
geode.MouseMoveEvent.listen(function(x, y)
    print("mouse", x, y)
    return false
end)
```

## ScrollWheelEvent

```lua
geode.ScrollWheelEvent.listen(callback: (xOffset: number, yOffset: number) -> boolean?, priority: number?) -> MouseInputListenerHandle
```

Listens to horizontal and vertical wheel offsets.

```lua
geode.ScrollWheelEvent.listen(function(xOffset, yOffset)
    print("scroll", xOffset, yOffset)
    return false
end)
```

## Listener handles

```lua
handle:disconnect() -> ()
```

Store the handle while the listener is active.
Handles also disconnect during garbage collection and runtime shutdown.
The optional priority works like other Geode event listeners.
Callback errors and timeouts are logged and let propagation continue.

## Related

- [Keyboard input](keyboard-input.md)
- [callbacks](callbacks.md)
- [globals](globals.md)
- [type stubs](type-stubs.md)
- [Getting started](../../getting-started/overview.md)

## Source

- `src/bindings/geode/GeodeMouseBinding.cpp`
- `tools/luau_codegen/extra_bindings/mouse.dluau`
