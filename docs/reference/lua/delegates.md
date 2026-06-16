# delegates

## Summary

Some C++ APIs take a delegate pointer, an object (list below) with virtual methods:

- touch delegates
- keyboard delegates
- scroll view delegates
- alert protocols
- similar interface delegates

In Luau, pass a table with method names as keys and Luau functions as values.
The runtime creates a C++ trampoline that calls the matching table function for each virtual method.

## Table shape

Each supported delegate has a Luau type stub.
The stub lists optional function fields, one per virtual method.
Include only the methods you care about.

```lua
layer:registerWithTouchDispatcher({
    ccTouchBegan = function(touch: CCTouch, event: CCEvent): boolean
        return true
    end,
    ccTouchEnded = function(touch: CCTouch, event: CCEvent)
        print("touch ended")
    end,
}, 0, false)
```

When a callback errors or times out, LuauAPI logs the failure and returns the method default.
When a method is missing from the table, LuauAPI returns the method default without logging.
`bool` returns `false`, `int` and `float`/`double` return `0`, `string` returns empty, and object methods return `nil`.
Method names and argument types match the C++ interface. Multi-touch variants use `CCSet`.

## Keyboard delegate

Use `CCKeyboardDelegate` when you need cocos dispatcher integration on a node or layer.
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
  - `CCAccelerometerDelegate`
  - `CCIMEDelegate`
  - `CCTextFieldDelegate`
  - (and others)
- Geode/game interfaces from Broma, such as:
  - alert protocols
  - scroll delegates
  - download callbacks

Generated trampolines live under `build/luauapi-gen/src/framework/callback/` and regenerate with the normal build.
See [Codegen](../../contributor/codegen/codegen.md).

## Limits

See [Limits and errors](../cpp/limits-and-errors.md) for callback budgets and orphan-registry caps.

## Related

- [Getting started](../../getting-started/overview.md)
- [callbacks](callbacks.md)
- [Keyboard input](keyboard-input.md)
- [game objects](game-objects.md)
- [type stubs](type-stubs.md)
- [Codegen](../../contributor/codegen/codegen.md)
- [globals](globals.md)

## Source

- `src/framework/callback/LuaDelegate.hpp`
- `build/luauapi-gen/delegate_specs.py` (generated at build time)
- `tools/luau_codegen/model/delegate_specs.py` (repo stub, not the runtime source)
- `tools/luau_codegen/cli/main.py`
