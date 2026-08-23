# hooks

## Summary

A hook runs your code before or after a game function.
You can read arguments, change arguments, skip the original, or change the return value.
`geode.hook` registers a callback and returns a handle.
Types match the generated stub `types/geode.d.luau`.

## geode.hook

```lua
geode.hook(target: string, callback: HookCallbackTable) -> HookHandle
```

Registers a callback on a target function. You must pass exactly two arguments.
An unknown target raises an error. The hook is enabled when you register it.

## Target id

The target id is `namespace.Class:method/argCount`.

- `namespace.Class` is the bound class, for example `geode.gd.GameManager`.
- `method` is the method name.
- `argCount` is the argument count, not counting `self`.

Examples: `geode.gd.MenuLayer:init/0`, `geode.gd.GameManager:setIntGameVariable/2`.

The id uses argument count only, not argument types.
Two C++ overloads with the same argument count would collide on one id,
so codegen picks a preferred overload or rejects the ambiguity at build time.

## HookCallbackTable

```lua
type HookCallbackTable = {
    before: ((...any) -> any?)?,
    after: ((...any) -> any?)?,
    priority: number?,
}
```

Provide at least one of `before` or `after`. `priority` orders callbacks on the same target, default `0`.

## The before callback

`before` receives `self` and the method arguments. Its return decides what happens:

- `nil` or nothing: run the original.
- `{ args = {...} }`: replace arguments (positional or named keys).
  Wrong types are logged and the original args are kept.
- `geode.skip(value)`: skip the original and use `value` as the return.
  For void methods use `geode.skip()`. Skip also suppresses all `after` callbacks for that invocation.
- Any other non-nil value: logged and ignored, original still runs.

```lua
geode.hook("geode.gd.GameManager:setIntGameVariable/2", {
    before = function(self, key, value)
        if key == "0027" and value == 0 then
            return { args = { key, 1 } }
        end
    end,
})
```

Argument overrides also accept named keys:

```lua
geode.hook("geode.gd.GameManager:setIntGameVariable/2", {
    before = function(self, key, value)
        if value < 0 then
            return { args = { key = key, value = 0 } }
        end
    end,
})
```

A failing or timed-out `before` behaves like returning nothing. The original still runs.

### Nil object arguments in overrides

When rewriting arguments, hook callbacks may pass nil for object pointer arguments.
Normal API calls reject nil object pointers. This only applies inside hook argument overrides.

## The after callback

`after` receives `self`, the method arguments, then the return value last.
Void methods pass no return value, so the callback ends with the last argument.
It runs only when the original C++ call runs on that invocation.
If `before` returns `geode.skip()`, neither the original nor any `after` callback runs.

- Return a new value to replace the return.
- Return the value you received, or `nil`, to keep it.
- Wrong types are logged and the original return is kept.

```lua
geode.hook("geode.gd.GameManager:getIntGameVariable/1", {
    after = function(self, key, value)
        if key == "0040" then return 1 end
        return value
    end,
})
```

## Priority and order

`priority` only matters when several hooks share the same target.
A lower value runs `before` first. A higher value runs `after` first.
On a tie, earlier registration wins for `before`, later registration wins for `after`.

## geode.skip

```lua
geode.skip(value: any?) -> any
```

Builds a skip marker for use as a `before` return.
The original does not run, the value becomes the return, and `after` callbacks do not run.
Use no argument for functions that return nothing.

```lua
geode.hook("geode.gd.GameManager:getIntGameVariable/1", {
    before = function(self, key)
        if blockedKey(key) then
            return geode.skip(0)
        end
    end,
})
```

## HookHandle

```lua
type HookHandle = {
    enable: (self: HookHandle) -> (boolean, string?),
    disable: (self: HookHandle) -> (boolean, string?),
    remove: (self: HookHandle) -> boolean,
    isEnabled: (self: HookHandle) -> boolean,
}
```

`enable` and `disable` return `true` on success.
When the handle is invalid or removed, they return `false` only.
When Geode enable or disable fails, they return `false` and an error string.

```lua
handle:disable()
handle:enable()
handle:remove()
print(handle:isEnabled())
```

## geode.fields

Per-node Lua storage. See [game objects](game-objects.md) for `geode.fields`, `m_fields`, and node placement rules.

## Prefer the right tool

A hook is a sharp tool. Reach for a built-in API first when one exists.
Do not hook `CCScheduler::update` for per-frame work.
Use [tasks and time](tasks.md) (`task.every`) or a node's `schedule` selector.

See [LuauAPI mod guidelines](../../mod_guidelines.md) for when hooks are the wrong choice,
and the [Geode SDK guidelines tips](https://docs.geode-sdk.org/mods/guidelines-tips/) for review rules.

Luau hooks call originals through Geode's tulip wrapper for the hooked address,
so they compose with C++ `$modify` mods on the same method.

## Native crashes

Bad hook returns can fault in C++ outside Lua errors.
See [Crash sidecar](../../contributor/internals/crash-sidecar.md).

## Limits

See [Getting started](../../getting-started/overview.md) for the main-thread rule.
See [Limits and errors](../cpp/limits-and-errors.md) for callback caps and deadlines.

## Related

- [Getting started](../../getting-started/overview.md)
- [LuauAPI mod guidelines](../../mod_guidelines.md)
- [tasks and time](tasks.md)
- [callbacks](callbacks.md)
- [game objects](game-objects.md)
- [globals](globals.md)

## Source

- `tools/luau_codegen/extra_bindings/hook.dluau`
- `tools/luau_codegen/emit/hooks.py`
- `tools/luau_codegen/emit/cxx_templates.py`
- `src/core/Config.hpp`
