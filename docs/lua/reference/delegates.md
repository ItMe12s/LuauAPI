# Reference: delegates

## Summary

Some C++ APIs take a **delegate pointer**, which is just an object with virtual methods (like for touch, keyboard, scroll views, or alert protocols). In Luau, pass a **table** with method names as keys and Luau functions as values.

The runtime creates a C++ trampoline (`LuaX` subclass) that calls the matching table function for each virtual method. You can leave out unsupported methods, missing ones use safe defaults (like `false` for `bool`).

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

Method names and argument types match the C++ interface. Multi-touch variants (`ccTouchesBegan`, etc.) use `CCSet` for the touch set.

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

Regenerate specs and C++ trampolines after Broma changes:

```powershell
python tools/scripts/generate_delegate_artifacts.py
```

Generated delegate trampolines live under the codegen output tree:

- `${LUAUAPI_GEN_DIR}/src/lua/bindings/framework/LuaDelegates.gen.hpp`
- `${LUAUAPI_GEN_DIR}/src/lua/bindings/framework/LuaDelegates.gen.cpp`

Regenerate with the main Broma codegen stamp (`luauapi_codegen`) or directly:

```bash
python -m luau_codegen --emit-delegates --bindings <bindings-dir> --out <gen-dir>
```

## Limits

| Limit | Value |
| --- | --- |
| Callback script budget | `50 ms` (`kHookScriptDeadlineMs`) per delegate method invocation |
| Orphan delegate registry | `4096` soft cap (`kMaxCallbackTrampolines`) |

## Related

- [Callbacks](callbacks.md)
- [Using game objects](../guide/using-game-objects.md)
- [Type stubs](type-stubs.md)

## Source

- `src/lua/bindings/framework/LuaDelegate.hpp`
- `tools/luau_codegen/model/delegate_specs.py`
- `tools/scripts/generate_delegate_artifacts.py`
