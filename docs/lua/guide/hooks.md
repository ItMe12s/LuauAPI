# Hooks

## Summary

A hook runs your code before or after a game function.
You can read arguments, change arguments, skip the original, or change the return value.

## The basic shape

```lua
local handle = geode.hook("geode.gd.MenuLayer:init/0", {
    after = function(self, result)
        print("MenuLayer opened")
        return result
    end,
})
```

Pass a target id string and a callback table. Signatures and limits: [Hooks reference](../reference/hooks.md).

## The target id

Format:

```text
namespace.Class:method/argCount
```

- `namespace.Class` is the bound class, for example `geode.gd.GameManager`.
- `method` is the method name.
- `argCount` is the argument count, not counting `self`.

Examples:

- `geode.gd.MenuLayer:init/0`
- `geode.gd.GameManager:setIntGameVariable/2`
- `geode.gd.GameManager:getGameVariable/1`

An unknown target raises an error.

## The callback table

Provide at least one of `before` or `after`. Optional fields:

- `before` runs before the original.
- `after` runs after the original.
- `priority` orders hooks on the same target. Default `0`.

## The before callback

`before` receives `self` and the method arguments.

```lua
geode.hook("geode.gd.GameManager:setIntGameVariable/2", {
    before = function(self, key, value)
        -- read or change the call here
    end,
})
```

Return value:

- `nil` or nothing: run the original.
- `{ args = {...} }`: replace arguments (positional or named keys). Wrong types are logged and the original args are kept.
- `geode.skip(value)`: skip the original and use `value` as the return. For void methods, use `geode.skip()`. Wrong skip types are logged and the original still runs.
- Any other non-nil value: logged and ignored, original still runs.

Replace arguments:

```lua
before = function(self, key, value)
    if key == "0027" and value == 0 then
        return { args = { key, 1 } }
    end
end
```

Skip the original:

```lua
before = function(self, key)
    if key == "0115" then
        return geode.skip(true)
    end
end
```

## The after callback

`after` receives `self`, the method arguments, then the return value last.

```lua
geode.hook("geode.gd.GameManager:getIntGameVariable/1", {
    after = function(self, key, value)
        if key == "0040" then
            return 1
        end
        return value
    end,
})
```

- Return a new value to replace the return.
- Return the value you received to keep it.
- Return `nil` to keep the original return.
- Wrong types are logged and the original return is kept.

## Priority and order

`priority` only matters when several hooks share the same target id.

| Callback | Runs first when |
| --- | --- |
| `before` | Priority is lower |
| `after` | Priority is higher |

Same `priority`: earlier registration wins for `before`, later registration wins for `after`.

```lua
geode.hook("geode.gd.GameManager:setIntGameVariable/2", {
    priority = -10,
    before = function(self, key, value) end,
})

geode.hook("geode.gd.GameManager:setIntGameVariable/2", {
    before = function(self, key, value) end,
})

geode.hook("geode.gd.GameManager:setIntGameVariable/2", {
    priority = 10,
    after = function(self, key, value, result)
        return result
    end,
})
```

The first hook runs its `before` first (`-10` before `0`). The third hook runs its `after` before any `after` at default priority `0`.

## The handle

`geode.hook` returns a handle. The hook is enabled when you register it.

```lua
local handle = geode.hook("geode.gd.MenuLayer:init/0", {
    after = function(self, result) return result end,
})

handle:disable()
handle:enable()
handle:remove()
print(handle:isEnabled())
```

Method details: [Hooks reference](../reference/hooks.md).

## Per object fields

Store data on the object with `self.m_fields` or `geode.fields(self)`.

```lua
geode.hook("geode.gd.MenuLayer:init/0", {
    after = function(self, result)
        local fields = geode.fields(self)
        fields.opens = (fields.opens or 0) + 1
        return result
    end,
})
```

See [Using game objects](using-game-objects.md).

## Limits and notes

Counts and the `50 ms` callback budget: [Hooks reference](../reference/hooks.md).
Hooks run on the main thread.
Shared runtime caps: [Limits and errors](../../cpp/limits-and-errors.md).

## Related

- [Hooks reference](../reference/hooks.md)
- [Using game objects](using-game-objects.md)

## Source

- `tools/luau_codegen/emit/cxx_templates.py`
- `tools/luau_codegen/emit/hooks.py`
- `tools/luau_codegen/emit/luau_types/`
- `src/lua/Config.hpp`
