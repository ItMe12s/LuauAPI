# Reference: callbacks

## Summary

Some C++ APIs take a callback or selector handler. LuauAPI bridges a Luau function into those slots at call time.

Three shapes are supported:

- **`std::function` / `Function` / `MiniFunction` / `Callback` arguments**: Pass a Luau function. The runtime wraps it so C++ can call it. Non-void return types are supported when all parameter and return types are bindable.
- **`SEL_*` selector handlers**: Cocos2d selector typedefs (`SEL_MenuHandler`, `SEL_SCHEDULE`, `SEL_CallFunc`, `SEL_CallFuncN`, `SEL_CallFuncND`, `SEL_CallFuncO`). Pass one Luau function with the matching signature instead of `(target, selector)` pairs where pairing applies.
- **Delegate tables**: Virtual interfaces passed as delegate pointers. See [Delegates](delegates.md).

Callbacks run on the main thread with the same script budget as hooks (`50 ms`).

## std::function-style callbacks

When a bound method or free function takes a `std::function<...>` (or Geode `Function` / `MiniFunction`,
or the `Callback` alias for `std::function<void()>`), pass a Luau function with a matching signature.

```lua
-- void callback
someObject:doLater(function(arg1: CCNode)
    print("called from C++", arg1)
end)

-- non-void return (example shape, actual APIs vary)
local ok = someObject:tryAction(function(layer: FLAlertLayer, accepted: boolean): boolean
    return accepted
end)
```

Supported callback arguments must:

- Use only bindable argument types (no nested callbacks).
- Use bindable return types when the C++ side is not `void` (`bool`, numbers, strings, enums, objects, and value types such as `CCPoint` / `RGBAColor`).

## Selector handlers

Cocos2d APIs normally take a target object and a selector. In Luau, pass one function with the signature implied by the selector type:

| C++ selector | Luau function shape |
| --- | --- |
| `SEL_MenuHandler` | `(sender: CCObject) -> ()` |
| `SEL_SCHEDULE` | `(dt: number) -> ()` |
| `SEL_CallFunc` | `() -> ()` |
| `SEL_CallFuncN` | `(node: CCNode) -> ()` |
| `SEL_CallFuncND` | `(node: CCNode, data: userdata) -> ()` |
| `SEL_CallFuncO` | `(obj: CCObject) -> ()` |

Menu example, `(CCObject* target, SEL_MenuHandler selector)` counts as **one** Luau argument:

```lua
local item = CCMenuItemSpriteExtra:create(normal, selected, disabled, function(sender: CCObject)
    print("menu item clicked", sender)
end)
```

Schedule example, `CCNode:schedule` / `scheduleOnce` take `(target, selector)` in C++, pass one function:

```lua
node:schedule(function(dt: number)
    print("tick", dt)
end, 1.0)
```

When a method has `(CCObject* target, SEL_* selector)` the pair collapses to one Luau argument.
Some schedule APIs accept a selector without an explicit target, pass the function alone and the handler object is used as the target.

## Lifetime

Each selector handler trampoline is retained and associated with an anchor `CCObject*`:

- The returned `CCObject*` when the method returns one.
- `self` for instance methods that do not return an object.
- The orphan registry for static calls and void-return methods (cleared on runtime shutdown).

When the anchor's retain count drops to one before `release`, all its handlers are cleaned up, like `geode.fields` tables.
The orphan registry has a soft cap of `4096`. If exceeded, a warning is logged, but all handlers are still kept.

`std::function` wrappers are held for the duration of the C++ call that received them (same `LuaCallback` lifetime as before).

## Limits

| Limit | Value |
| --- | --- |
| Callback script budget | `50 ms` (`kHookScriptDeadlineMs`) |
| Orphan handler registry | `4096` soft cap (`kMaxCallbackTrampolines`). Warns once, never drops |

## Related

- [Delegates](delegates.md)
- [Hooks](hooks.md)
- [Tasks and time](../guide/tasks-and-time.md)
- [Using game objects](../guide/using-game-objects.md)

## Source

- `src/lua/bindings/framework/LuaCallback.hpp`
- `src/lua/bindings/framework/LuaMenuHandler.hpp`
- `src/lua/bindings/framework/LuaSelectorHandler.hpp`
- `src/lua/bindings/framework/LuaTrampolineRegistry.hpp`
- `tools/luau_codegen/convert/marshalling.py`
- `tools/luau_codegen/convert/type_map.py`
