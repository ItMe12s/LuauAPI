# Sharing APIs between mods

## Summary

All mods share one runtime and one global table.
Functions and values published from Luau or registered from C++ are visible to other mods later.
Use this to let one mod expose an API to others.

The runnable sample pair is `dummy-mods/provider/` (publishes from C++) and `dummy-mods/consumer/` (consumes it).

## Publishing from C++

Direct native registration publishes C++ functions and values under the caller's full mod id.
It dispatches through Geode events, so a provider can publish with LuauAPI as an optional dependency.
When LuauAPI is not loaded, each register call returns `Err("Unable to call method")`.
See [Native C++ registration](../cpp/native-registration.md) for setup, supported types, registration rules, and callback behavior.

The registering mod's own scripts can read this table through `_G[geode.Mod.getID()]`.
Scripts from other mods use the registering mod's explicit id:

```lua
local MathMod = _G["cool.mathmod"]
print(MathMod.math.isEven(6))
print(MathMod.math.divide(8, MathMod.math.defaultDivisor))
```

## Publishing from Luau

Put your API in one table on `_G`, keyed by your full Geode mod id.

```lua
_G["cool.mathmod"] = {
    math = {
        defaultDivisor = 2,
        isEven = function(value)
            return value % 2 == 0
        end,
    },
}
```

- Key by your full mod id so tables cannot collide. This is a convention. See [LuauAPI mod guidelines](../../mod_guidelines.md).
- Keep the table shape stable once others depend on it.
- Publish once, after your API is ready.

## Consuming

Read the other mod's table by its id. Always index `_G` with the key, never as a bare global name.
The global table is a safe environment, so a bare read can cache `nil` forever. Indexing `_G` stays dynamic.

```lua
local MathMod = _G["cool.mathmod"]
```

Ask consumers to depend on your mod with a Geode version range for compatibility checks if needed.

## Handling load order

The provider may run after you, so the first read can be `nil`.
Do not busy wait. A `repeat` loop never works here,
because your script runs in one call with a time budget while the provider runs in a later call.
Poll with `task` instead, and give up after a limit:

```lua
local function whenReady(id, callback)
    local api = _G[id]
    if api then
        callback(api)
        return
    end
    local tries = 0
    local handle
    handle = task.every(0.1, function()
        tries += 1
        if tries >= 100 then
            handle:cancel()
            error("timed out waiting for " .. id)
        end
        local found = _G[id]
        if found then
            handle:cancel()
            callback(found)
        end
    end)
end

whenReady("cool.mathmod", function(MathMod)
    print(MathMod.math.add(2, 3))
end)
```

## Notes

- Values stay for the life of the runtime. There is no auto cleanup.
- The poll runs on the game tick. Poll timing follows task rules. See [tasks and time](tasks.md).
- `require` cannot reach another mod. It is sandboxed to your own resources. See [modules](modules.md).

## Related

- [globals](globals.md)
- [modules](modules.md)
- [tasks and time](tasks.md)
- [Native C++ registration](../cpp/native-registration.md)
- [Limits and errors](../cpp/limits-and-errors.md)
- [LuauAPI mod guidelines](../../mod_guidelines.md)
- [Getting started](../../getting-started/overview.md)

## Source

- `include/NativeRegistration.hpp`
- `src/api.cpp`
- `src/core/Runtime.cpp`
- `src/bindings/task/TaskBinding.cpp`
