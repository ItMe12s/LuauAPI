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

When a callback errors, times out, or is missing, LuauAPI logs the failure and returns the method default.
`bool` returns `false`, `int` returns `0`, `string` returns empty, and object methods return `nil`.
Method names and argument types match the C++ interface. Multi-touch variants use `CCSet`.

## Lifetime and anchoring

Delegate trampolines use the same retention model as menu handlers:

- When a bound method returns a `CCObject*`, that object anchors the delegate.
- For instance methods that do not return an object, `self` anchors the delegate.
- For static calls and void-return methods, delegates go into the orphan registry, cleared on runtime shutdown.

When the anchor's retain count drops to one before `release`, anchored delegates are cleaned up.
The orphan registry has a soft cap of `4096` (warns once, never drops entries).

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

Delegate method invocations run under a script budget, and orphan delegate bridges have a soft registry cap.
See [Limits and errors](../cpp/limits-and-errors.md).

## Related

- [Callbacks](callbacks.md)
- [Game objects](game-objects.md)
- [Type stubs](type-stubs.md)
- [Codegen](../../contributor/codegen/codegen.md)

## Source

- `src/framework/callback/LuaDelegate.hpp`
- `build/luauapi-gen/delegate_specs.py` (generated at build time)
- `tools/luau_codegen/model/delegate_specs.py` (repo stub, not the runtime source)
- `tools/luau_codegen/cli/main.py`
