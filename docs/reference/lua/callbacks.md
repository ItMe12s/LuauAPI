# callbacks

## Summary

Some C++ APIs take a callback or selector handler.
LuauAPI bridges a Luau function into those slots at call time.

The supported callback shapes are:

- `std::function` style arguments
- `SEL_*` selector handlers
- Delegate tables

Callbacks run on the main thread with the same script budget as hooks.
If a callback raises an error, LuauAPI logs the failure and applies the callback site's fallback.
Selector, menu, delegate, setting, web, and permission callbacks keep their registration lifetime.
Task intervals and ImGui draw callbacks are removed after an error to avoid log spam.

## std::function style callbacks

When a bound method takes a `std::function<...>` (or Geode `Function` / `MiniFunction`,
or the `Callback` alias for `std::function<void()>`), pass a Luau function with a matching signature.

```lua
-- void callback
someObject:doLater(function(arg1: CCNode)
    print("called from C++", arg1)
end)

-- non-void return (shape only, real APIs vary)
local ok = someObject:tryAction(function(layer: FLAlertLayer, accepted: boolean): boolean
    return accepted
end)
```

Supported callback arguments must use only bindable argument types (no nested callbacks)
and bindable return types when the C++ side is not `void`.

Supported bindable return types include:

- booleans
- numbers
- strings
- enums
- objects
- value types (such as `CCPoint` or `RGBAColor`).

## Selector handlers

Cocos2d APIs normally take a target object and a selector.
In Luau, pass one function with the signature implied by the selector type.

| C++ selector | Luau function shape |
| --- | --- |
| `SEL_MenuHandler` | `(sender: CCObject) -> ()` |
| `SEL_SCHEDULE` | `(dt: number) -> ()` |
| `SEL_CallFunc` | `() -> ()` |
| `SEL_CallFuncN` | `(node: CCNode) -> ()` |
| `SEL_CallFuncND` | `(node: CCNode, data: userdata) -> ()` |
| `SEL_CallFuncO` | `(obj: CCObject) -> ()` |

When a method has `(CCObject* target, SEL_* selector)`, the pair collapses to one Luau argument.

```lua
local item = CCMenuItemSpriteExtra:create(normal, selected, disabled, function(sender: CCObject)
    print("menu item clicked", sender)
end)

node:schedule(function(dt: number)
    print("tick", dt)
end, 1.0)
```

Some schedule APIs accept a selector without an explicit target.
Pass the function alone, and the handler object is used as the target.

## Delegate tables

Virtual interfaces passed as delegate pointers take a Luau table. See [Delegates](delegates.md).

## Lifetime

Each selector handler trampoline is retained and associated with an anchor `CCObject*`:

- The returned `CCObject*` when the method returns one.
- `self` for instance methods that do not return an object.
- The orphan registry for static calls and void-return methods, cleared on runtime shutdown.

When the anchor's retain count drops to one before `release`, all its handlers are cleaned up,
like `geode.fields` tables. `std::function` wrappers are held for the duration of the C++ call that received them.
Task and ImGui handles cancel their callback when you call `:cancel()` or when the handle userdata is collected.

## Limits

Callbacks run under a script budget, and orphan handler bridges have a soft registry cap.

See [Limits and errors](../cpp/limits-and-errors.md) for caps and error strings.

## Related

- [Delegates](delegates.md)
- [Hooks](hooks.md)
- [Tasks and time](tasks.md)
- [Game objects](game-objects.md)
- [Limits and errors](../cpp/limits-and-errors.md)

## Source

- `src/framework/callback/LuaCallback.hpp`
- `src/framework/callback/LuaMenuHandler.hpp`
- `src/framework/callback/LuaSelectorHandler.hpp`
- `src/framework/callback/LuaTrampolineRegistry.hpp`
- `tools/luau_codegen/convert/marshalling.py`
- `tools/luau_codegen/convert/type_map.py`
