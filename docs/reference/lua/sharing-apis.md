# Sharing APIs between mods

## Summary

All mods share one runtime and one global table.
Functions and values published from Luau or registered from C++ are visible to other mods later.
Use this to let one mod expose an API to others.

## Publishing from C++

Direct native registration publishes C++ functions and values under the caller's full mod id.
It dispatches through Geode events, so a provider can publish with LuauAPI as an optional dependency.
When LuauAPI is not loaded, each register call returns `Err("Unable to call method")`.
The registering mod's own scripts can read this table through `_G[geode.Mod.getID()]`.
Scripts from other mods use the registering mod's explicit id:

```lua
local Provider = _G["provider.mod"]
print(Provider.math.isEven(6))
print(Provider.math.divide(8, Provider.math.defaultDivisor))
```

See [Native C++ registration](../cpp/native-registration.md) for setup, supported types, registration rules, and callback behavior.

## Publishing from Luau

Put your API in one table on `_G`, keyed by your full Geode mod id.

```lua
_G["imes.luauapi"] = {
    math = {
        defaultDivisor = 2,
        isEven = function(value)
            return value % 2 == 0
        end,
    },
}
```

- You have to inform other people to use Geode dependency version ranges for compatibility checks if needed.
- Keep the table shape stable once others depend on it.
- Publish once, after your API is ready.

## Consuming

Read the other mod's table by its id. Always index `_G` with the key, never as a bare global name.
The global table is a safe environment, so a bare read can cache `nil` forever. Indexing `_G` stays dynamic.

```lua
local OtherMod = _G["other.mod.id"]
```

## Handling load order

The provider may run after you, so the first read can be `nil`.
Do not busy wait. A `repeat` loop never works here,
because your script runs in one call with a time budget while the provider runs in a later call.
Poll with `task` instead.

```lua
local function whenReady(id, callback)
    local api = _G[id]
    if api then
        callback(api)
        return
    end
    local handle
    handle = task.every(0.1, function()
        local found = _G[id]
        if found then
            handle:cancel()
            callback(found)
        end
    end)
end

whenReady("cool.mathmod", function(MathMod)
    print(MathMod.add(2, 3))
end)
```

## Notes

- This is a convention, not enforced. Key by your full mod id to avoid collisions.
- Values stay for the life of the runtime. There is no auto cleanup.
- The poll runs on the game tick. Poll timing follows task rules. See [tasks and time](tasks.md).
- `require` cannot reach another mod. It is sandboxed to your own resources. See [modules](modules.md).

## Related

- [Getting started](../../getting-started/overview.md)
- [C++ API reference](../cpp/api-reference.md)
- [Limits and errors](../cpp/limits-and-errors.md)
- [modules](modules.md)
- [tasks and time](tasks.md)
- [globals](globals.md)
- [LuauAPI mod guidelines](../../mod_guidelines.md)

## Source

- `include/NativeRegistration.hpp`
- `src/core/Runtime.cpp`
- `src/api.cpp`
- `src/bindings/task/TaskBinding.cpp`
