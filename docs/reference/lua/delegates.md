# delegates

## Summary

Some C++ APIs take a delegate pointer for an interface with virtual methods:

- touch delegates
- keyboard delegates
- scroll view delegates
- alert protocols

In Luau, pass a table with method names as keys and Luau functions as values.
The runtime builds a C++ trampoline that calls the matching table function for each virtual method.

## Table shape

Each supported delegate has a Luau type stub.
The stub lists optional function fields, one per virtual method.
Include only the methods you care about.

Touch delegates register on the dispatcher, not on the layer:

```lua
local director = geode.cocos2d.CCDirector.sharedDirector()
if not director then return end

director:getTouchDispatcher():addTargetedDelegate({
    ccTouchBegan = function(touch: CCTouch, event: CCEvent): boolean
        return true
    end,
    ccTouchEnded = function(touch: CCTouch, event: CCEvent)
        print("touch ended")
    end,
}, 0, false)
```

Default behavior on error or missing method:

- If a callback errors or times out, LuauAPI logs the failure and returns the method default.
- A missing method returns the default without logging.
- Defaults: `bool` returns `false`, `int`/`float`/`double` return `0`, `string` returns empty, object methods return `nil`.

Method names and argument types match the C++ interface. Multi-touch variants use `CCSet`.

## Keyboard delegate

Pass a `CCKeyboardDelegate` table to `CCKeyboardDispatcher:addDelegate`.
Use [Keyboard input](keyboard-input.md) for the global Geode event stream.

```lua
local director = geode.cocos2d.CCDirector.sharedDirector()
if not director then return end

director:getKeyboardDispatcher():addDelegate({
    keyDown = function(key: number, dt: number)
        print("down", key, dt)
    end,
    keyUp = function(key: number, dt: number)
        print("up", key, dt)
    end,
})
```

Dispatchers also expose `removeDelegate` and `forceRemoveDelegate` to unregister.

## Lifetime and anchoring

Delegate trampolines use the same anchor and orphan registry rules as other callbacks.
See [callbacks](callbacks.md) for retention, cleanup, and caps.

## Delegate return values

When a bound method returns a delegate pointer:

- If the native object is a Luau-bound trampoline, the runtime pushes the same table you passed in.
- Otherwise it pushes `nil` (a native C++ delegate, not round-trippable to Luau).

## Supported interfaces

Supported interfaces include:

- Cocos2d input delegates:
  - `CCTouchDelegate`
  - `CCKeyboardDelegate`
  - `CCKeypadDelegate`
  - `CCMouseDelegate`
  - `CCIMEDelegate`
  - `CCTextFieldDelegate`
  - `CCEditBoxDelegate`
- Geode/game interfaces from Broma, such as:
  - alert protocols
  - scroll delegates
  - download callbacks

Geode and game interfaces follow the Broma `*Delegate`/`*Protocol` types.
See the exact members in [type stubs](type-stubs.md).

The dominant Geode pattern is a delegate-typed field:

```lua
popup.m_delegate = {
    setIDPopupClosed = function(popup: SetIDPopup, id: number)
        print("closed", id)
    end,
}
```

Generated trampolines live under `build/luauapi-gen/src/framework/callback/` and regenerate with the normal build.
See [Codegen](../../contributor/codegen/codegen.md).

## Limits

See [Limits and errors](../cpp/limits-and-errors.md) for callback budgets and orphan-registry caps.

## Related

- [callbacks](callbacks.md)
- [Keyboard input](keyboard-input.md)
- [game objects](game-objects.md)
- [type stubs](type-stubs.md)
- [globals](globals.md)
- [Codegen](../../contributor/codegen/codegen.md)

## Source

- `src/framework/callback/LuaDelegate.hpp`
- `tools/luau_codegen/model/delegate_specs.py`
- `tools/luau_codegen/emit/delegates.py`
- `tools/luau_codegen/cli/main.py`
