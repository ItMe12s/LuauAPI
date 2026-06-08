# Reference: delegates

## Summary

Some C++ APIs take a **delegate pointer**, which is just an object with virtual methods
(like for touch, keyboard, scroll views, or alert protocols).
In Luau, pass a **table** with method names as keys and Luau functions as values.

The runtime creates a C++ trampoline (`LuaX` subclass) that calls the matching table function for each virtual method.
You can leave out unsupported methods. Missing ones use safe defaults (like `false` for `bool`).

## Table shape

Each supported delegate has a Luau type stub (see generated `types/` or [Type stubs](type-stubs.md)).
The stub lists optional function fields, one per virtual method.

Example: touch delegate passed to a layer that registers touch handling:

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

Only include methods you care about.
Missing keys are not called from C++ unless the engine invokes that virtual (defaults apply inside the trampoline).

When a callback errors, times out, or is missing, LuauAPI logs the failure and returns the method default.
`bool` methods return `false`. `int` methods return `0`. `string` methods return an empty string.
Object methods return `nil`. See [Callbacks](callbacks.md).

Method names and argument types match the C++ interface.
Multi-touch variants (`ccTouchesBegan`, etc.) use `CCSet` for the touch set.

## Lifetime and anchoring

Delegate trampolines use the same retention model as menu handlers:

- When a bound method **returns** a `CCObject*`, that object anchors the delegate.
- For **instance methods** that do not return an object, `self` anchors the delegate.
- For **static** calls and void-return methods, delegates go into the orphan registry (cleared on runtime shutdown).

When the anchor's retain count drops to one before `release`, anchored delegates are cleaned up.
The orphan registry has a soft cap of `4096` (warns once, never drops entries).

## Delegate return values

When a bound method **returns** a delegate pointer:

- If the native object is a Luau-bound trampoline, the runtime pushes the **same table** you originally passed in.
- Otherwise it pushes **`nil`** (native C++ delegate, not round-trippable to Luau).

## Supported interfaces

Codegen covers:

- cocos2d input delegates:
  - `CCTouchDelegate`
  - `CCKeyboardDelegate`
  - `CCKeypadDelegate`
  - `CCMouseDelegate`
  - `CCAccelerometerDelegate`
  - `CCIMEDelegate`
  - `CCTextFieldDelegate`
  - and others
- Geode/game interfaces from Broma, such as:
  - alert protocols
  - scroll delegates
  - download callbacks
  - and similar interfaces

Generated delegate trampolines live under `build/luauapi-gen/src/lua/bindings/framework/` as `LuaDelegates.gen.hpp` and `LuaDelegates.gen.cpp`.
Regenerate with the normal build (`luauapi_codegen`) or see [Codegen](../../dev/codegen.md).

## Limits

Delegate method invocations run under a script budget, and orphan delegate bridges have a soft registry cap.
See [Limits and errors](../../cpp/limits-and-errors.md) for the values.

## Related

- [Callbacks](callbacks.md)
- [Codegen](../../dev/codegen.md)
- [Using game objects](../guide/using-game-objects.md)
- [Type stubs](type-stubs.md)

## Source

- `src/lua/bindings/framework/LuaDelegate.hpp`
- `build/luauapi-gen/delegate_specs.py` (generated at build time, path set by `LUAUAPI_DELEGATE_SPECS_OUT` in CMake)
- `tools/luau_codegen/model/delegate_specs.py` (repo stub, not the runtime source)
- `tools/luau_codegen/cli/main.py`
