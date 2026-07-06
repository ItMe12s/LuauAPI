# callbacks

## Summary

Some C++ APIs take a callback or selector handler.
LuauAPI bridges a Luau function into those slots at call time.

The supported callback shapes are:

- `std::function` style arguments
- `SEL_*` selector handlers
- Delegate tables

Callbacks run on the main thread under the script budget.
See [Getting started](../../getting-started/overview.md) and [Limits and errors](../cpp/limits-and-errors.md).
If a callback raises an error, LuauAPI logs the failure and applies the callback site's fallback.
Registration lifetime varies by callback kind. See Lifetime below.

## std::function style callbacks

When a bound method takes a C++ callback (such as a `std::function`, Geode `Function`, or `Callback` alias),
pass a Luau function with a matching signature.

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
- [enums](enums.md)
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

Virtual interfaces passed as delegate pointers take a Luau table. See [delegates](delegates.md).

## Lifetime

Each selector handler trampoline is retained and associated with an anchor `CCObject*`:

- The returned `CCObject*` when the method returns one.
- `self` for instance methods that do not return an object.
- The orphan registry for static calls and void-return methods, cleared on runtime shutdown.

When the anchor's retain count drops to one before `release`, all its handlers are cleaned up,
like `geode.fields` tables. `std::function` wrappers are held for the duration of the C++ call that received them.
Task and ImGui handles cancel their callback when you call `:cancel()` or when the handle userdata is collected.

Selector, menu, delegate, setting, web, and permission callbacks keep their registration lifetime.
Task intervals and ImGui draw callbacks are removed after an error to avoid log spam.

## Shutdown

While the runtime is shutting down, `LuaCallback::invoke` returns `false` immediately and does not enter Lua.
Orphan selector trampolines are released through a runtime shutdown hook.
See [Runtime](../../contributor/internals/runtime.md) Shutdown for LIFO hook order.

## Limits

See [Limits and errors](../cpp/limits-and-errors.md) for callback budgets and orphan-handler caps.

## Related

- [Getting started](../../getting-started/overview.md)
- [delegates](delegates.md)
- [hooks](hooks.md)
- [tasks and time](tasks.md)
- [enums](enums.md)
- [game objects](game-objects.md)
- [mod](mod.md)
- [web](web.md)
- [imgui](imgui.md)
- [Limits and errors](../cpp/limits-and-errors.md)

## Source

- `src/framework/callback/LuaCallback.hpp`
- `src/framework/callback/LuaCocosHandler.hpp`
- `src/framework/callback/LuaTrampolineRegistry.hpp`
- `tools/luau_codegen/convert/marshalling.py`
- `tools/luau_codegen/convert/type_map.py`
