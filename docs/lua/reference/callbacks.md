# Reference: callbacks

## Summary

Some C++ APIs take a callback or menu handler. LuauAPI bridges a Luau function into those slots at call time.

Two shapes are supported:

- **`std::function` / `Function` / `MiniFunction` arguments**: Pass a Luau function as the argument. The runtime will wrap it so C++ can call it.
- **`SEL_MenuHandler` menu handlers**: If a method has `(CCObject* target, SEL_MenuHandler selector)`, just pass one Luau function like `(sender: CCObject) -> ()`. The runtime creates an object to connect the function and uses `menu_selector`.

Callbacks run on the main thread with the same script budget as hooks (`50 ms`).

## std::function-style callbacks

When a bound method or free function takes a `std::function<void(...)>` (or Geode `Function` / `MiniFunction`) argument,
pass a Luau function with a matching signature.

```lua
-- Example shape (actual APIs vary by method)
someObject:doLater(function(arg1: CCNode)
    print("called from C++", arg1)
end)
```

Supported callback arguments must:

- Return `void` on the C++ side.
- Use only bindable argument types (no nested callbacks).

## Menu handler callbacks

Cocos2d menu APIs normally take a target object and a `SEL_MenuHandler` selector. In Luau, pass one function that receives the sender:

```lua
local item = CCMenuItemSpriteExtra:create(normal, selected, disabled, function(sender: CCObject)
    print("menu item clicked", sender)
end)
```

The `(CCObject* target, SEL_MenuHandler selector)` pair counts as **one** Luau argument.

## Lifetime

Each menu handler trampoline is retained and associated with an anchor `CCObject*`:

- The returned `CCObject*` when the method returns one.
- `self` for instance methods that do not return an object.
- The orphan registry for static calls and void-return methods (cleared on runtime shutdown).

When the anchor's retain count drops to one before `release`, all its handlers are cleaned up, like `geode.fields` tables.
The orphan registry has a soft cap of `4096`. If exceeded, a warning is logged, but all handlers are still kept.

## Limits

| Limit | Value |
| --- | --- |
| Callback script budget | `50 ms` (`kHookScriptDeadlineMs`) |
| Orphan menu handler registry | `4096` soft cap (`kMaxCallbackTrampolines`). Warns once, never drops |

## Related

- [Hooks](hooks.md): intercept game methods with `geode.hook`
- [Tasks and time](../guide/tasks-and-time.md): schedule Luau callbacks with `task`
- [Using game objects](../guide/using-game-objects.md)

## Source

- `src/lua/bindings/framework/LuaCallback.hpp`
- `src/lua/bindings/framework/LuaMenuHandler.hpp`
- `src/lua/bindings/framework/LuaMenuHandler.cpp`
- `tools/luau_codegen/convert/marshalling.py`
- `tools/luau_codegen/convert/type_map.py`
