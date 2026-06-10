# sharing APIs between mods

## Summary

All mods share one runtime and one global table.
A value one script puts on a global is visible to other mods later.
Use this to let one mod expose a Luau API to others.

## Publishing

Put your API in one table on `_G`, keyed by your full Geode mod id.

```lua
_G["imes.luauapi"] = {
    version = 1,
    doThing = function(...) end,
}
```

- Add a `version` field for compatibility checks.
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
- The poll runs on the game tick, so it pauses when the game pauses.
- `require` cannot reach another mod. It is sandboxed to your own resources. See [Modules](modules.md).

## Related

- [Modules](modules.md)
- [Tasks and time](tasks.md)
- [Globals](globals.md)

## Source

- `src/core/Runtime.cpp`
- `src/api.cpp`
- `src/bindings/task/TaskBinding.cpp`
