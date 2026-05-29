# Hooks and modify

## Summary

A hook lets your script run code before or after a game function. You can read the arguments, change the arguments, skip the original, or change the return value. Hooks are the main way scripts change the game.

## The basic shape

Use `geode.hook` or `geode.modify`. They are the same function, so choose whichever name you prefer.

```lua
local handle = geode.hook("geode.gd.MenuLayer:init/0", {
    after = function(self, result)
        print("MenuLayer opened")
        return result
    end,
})
```

The first argument is the target id, and the second argument is a callback table.

## The target id

The target id names the function you want to hook. Its format is:

```text
namespace.Class:method/argCount
```

- `namespace.Class` is the bound class, for example `geode.gd.GameManager`.
- `method` is the method name.
- `argCount` is the number of arguments the method takes, not counting `self`.

Examples taken from real scripts:

- `geode.gd.MenuLayer:init/0`
- `geode.gd.GameManager:setIntGameVariable/2`
- `geode.gd.GameManager:getGameVariable/1`

A target that does not exist causes the call to raise an error.

## The callback table

The table can hold three fields. All of them are optional, but you must provide at least one of `before` or `after`.

- `before` runs before the original.
- `after` runs after the original.
- `priority` is a number that orders callbacks. The default is `0`.

## The before callback

`before` runs first, and it receives `self` followed by the method arguments.

```lua
geode.hook("geode.gd.GameManager:setIntGameVariable/2", {
    before = function(self, key, value)
        -- read or change the call here
    end,
})
```

A `before` callback controls what happens next through its return value.

### Return nothing to proceed

Return `nil` or nothing, and the original runs as normal.

### Change the arguments

Return a table with an `args` field, and the values inside replace the method arguments.

```lua
before = function(self, key, value)
    if key == "0027" and value == 0 then
        return { args = { key, 1 } }
    end
end
```

The `args` list can be positional, as shown above. It can also use the argument names as keys. A positional list must hold one value per method argument.

### Skip the original

Return `geode.skip(value)`. The original does not run, and the value you pass becomes the return value.

```lua
before = function(self, key)
    if key == "0115" then
        return geode.skip(true)
    end
end
```

For a function that returns nothing, call `geode.skip()` with no value.

### Invalid returns

Any other non nil return is ignored. The runtime logs a warning and runs the original.

## The after callback

`after` runs after the original. It receives `self`, then the method arguments, then the return value as the final argument.

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

To change the return value, return a new value. To keep it, return the value you received. Return `nil` to leave the original return in place.

## Priority and order

`priority` orders callbacks when more than one hook targets the same function.

- For `before` callbacks, a lower priority runs first.
- For `after` callbacks, a higher priority runs first.

When two callbacks share a priority, install order breaks the tie. This behavior is defined in the generated hook runtime.

## The handle

`geode.hook` returns a handle that you use to control the hook later.

```lua
local handle = geode.hook("geode.gd.MenuLayer:init/0", {
    after = function(self, result) return result end,
})

print(handle:isEnabled())
handle:disable()
handle:enable()
handle:remove()
```

- `handle:enable()` turns the hook on and returns a boolean.
- `handle:disable()` turns the hook off and returns a boolean.
- `handle:remove()` removes the callbacks for this handle and returns a boolean.
- `handle:isEnabled()` reports whether the hook is on and returns a boolean.

A hook is installed and enabled the first time you register it.

## Per object fields

Inside a hook you often want to store data on the object. Use `self.m_fields` or `geode.fields(self)`. Both give you a plain table tied to that object.

```lua
geode.hook("geode.gd.MenuLayer:init/0", {
    after = function(self, result)
        local fields = geode.fields(self)
        fields.opens = (fields.opens or 0) + 1
        return result
    end,
})
```

See [Using game objects](using-game-objects.md) for more.

## Limits and notes

- Total hook callbacks across all targets: `4096`.
- Hook callbacks per target: `64`.
- Each callback runs with a `50 ms` budget.
- Callbacks run on the main thread.

## Related

- [Hooks reference](../reference/hooks.md)
- [Using game objects](using-game-objects.md)

## Source

- `tools/luau_codegen/cxx_templates.py`
- `tools/luau_codegen/hooks.py`
- `tools/luau_codegen/emit_luau_types.py`
- `src/lua/Config.hpp`
