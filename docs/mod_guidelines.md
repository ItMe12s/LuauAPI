# LuauAPI mod guidelines

## Summary

These rules sit on top of the [Geode SDK Mod Guidelines](https://docs.geode-sdk.org/mods/guidelines/).
Read those first. They are the real Index rules.

This page is our LuauAPI overlay for mods that run Luau scripts. It is not official Geode policy yet.
Still, breaking these rules can make your mod look unsafe, unstable, or impossible to review.
Severe cases can still lead to real Geode Index rejection, delisting, or bans.

## Everything is situational

These rules are not magic law. Context matters.
A tiny joke mod and a large renderer mod do not need the same review depth.

Index moderators still have final say.

The goal is simple:

- Do not crash the game.
- Do not harm other mods.
- Do not abuse shared runtime state.
- Do not ship code that looks like malware.

## Rejection rules

A mod found breaking these rules will usually be rejected until fixed.
Repeated or severe abuse can also become a real Index problem.

### `reject-bad-type`

Do not reuse one variable for unrelated types, objects, or meanings.

Luau code gets hard to review when one name changes shape five times.
Use a new local name when the value becomes a different thing.

Bad:

```lua
local thing = geode.Mod.getID()
thing = gd3d.Transform.new()
thing = thing:position()
```

Good:

```lua
local modId = geode.Mod.getID()
local transform = gd3d.Transform.new()
local position = transform:position()
```

### `reject-fatal-return`

Hooks and callbacks must return required values.

A missing return can corrupt the call path. Some failures can crash outside normal Lua errors and may escape the Geode crash handler.

Bad:

```lua
geode.hook("geode.gd.GameManager:getIntGameVariable/1", {
    after = function(self, key, value)
        if key == "0040" then
            return 1
        end
    end,
})
```

Good:

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

### `reject-web-loadstring`

Never pipe web content into `loadstring`. No exceptions.

This is an exploit chain for remote code execution. Use data formats like JSON for remote data.

Bad:

```lua
geode.utils.web.get("https://example.com/update.luau", function(response, err)
    if not response then
        return
    end

    local text = response:text()
    local fn = loadstring(text)
    fn()
end)
```

Good:

```lua
geode.utils.web.get("https://example.com/config.json", function(response, err)
    if not response then
        print(err)
        return
    end

    local config, parseErr = response:json()
    if not config then
        print(parseErr)
        return
    end

    print(config.name)
end)
```

### `reject-bad-global`

Respect `_G`. Do not add bare globals. Do not use names like `_VAR` or `_SOMETHING`.

All mods share one runtime and one global table. Bare globals collide with other mods.

Names that start with `_` also look reserved for Lua internals.

Bad:

```lua
CoolApi = {}
_STATE = {}
```

Good:

```lua
_G["your.modid"] = {
    version = 1,
}
```

Even this should be rare. Prefer modules and locals when you do not need a public API.
See [Sharing APIs between mods](reference/lua/sharing-apis.md).

### `reject-gpu-maxxing`

Do not abuse the 3D renderer without meaningful user value.

A Luau mod should not turn a menu into a benchmark. If it lags mid range devices like an iPhone 15 or RTX 3050, it is not ready.

Bad:

```lua
for i = 1, 10000 do
    viewport:addMesh(mesh, gd3d.Transform.new(), material)
end
```

Good:

```lua
for i = 1, math.min(itemCount, 80) do
    viewport:addMesh(mesh, transforms[i], material)
end
```

See [gd3d](reference/lua/gd3d.md) and [Limits and errors](reference/cpp/limits-and-errors.md).

### `reject-cpu-maxxing`

Optimize code that burns script budgets.

Hooks, tasks, callbacks, web handlers, and draw callbacks run under time limits.

If your mod keeps hitting those limits, the code needs work.

Bad:

```lua
task.every(0, function()
    for i = 1, 1000000 do
        updateEverything(i)
    end
end)
```

Good:

```lua
local index = 1

task.every(0.05, function()
    for _ = 1, 200 do
        updateOne(index)
        index += 1
    end
end)
```

See [Limits and errors](reference/cpp/limits-and-errors.md).

### `reject-bad-gc`

Do not leak handles, nodes, callbacks, or long lived references.

The garbage collector can only clean unreachable values. If you keep old objects in tables forever, the leak is yours.

Bad:

```lua
local handles = {}

task.every(1, function()
    table.insert(handles, task.delay(10, function() end))
end)
```

Good:

```lua
local handle = task.every(1, function()
    print("tick")
end)

task.delay(10, function()
    handle:cancel()
end)
```

### `reject-keyword`

Do not use Lua reserved keywords or LuauAPI globals as variable names.

Keywords do not parse. Global names hide APIs and make code confusing.

Lua reserved keywords include: `and`, `break`, `continue`, `do`, `else`, `elseif`, `end`, `false`, `for`,
`function`, `if`, `in`, `local`, `nil`, `not`, `or`, `repeat`, `return`, `then`, `true`, `until`, and `while`.

Bad:

```lua
local task = {}
task.delay(1, function()
    print("never runs")
end)
```

Good:

```lua
local schedulerState = {}
task.delay(1, function()
    print(schedulerState)
end)
```

### `reject-scope`

Default to `local`.

Local access is faster and safer. Globals pollute the shared workspace and slowdown the runtime.

Bad:

```lua
score = 0

function addScore(amount)
    score += amount
end
```

Good:

```lua
local score = 0

local function addScore(amount)
    score += amount
end
```

### `reject-truthy-falsy`

Do not mix up `false`, `nil`, `0`, empty strings, and game objects.

In Lua, only `false` and `nil` are falsy. The number `0`, empty string `""`, and userdata are truthy.

Bad:

```lua
if not coins then
    coins = 1
end
```

Good:

```lua
if coins == nil then
    coins = 1
end
```

## Other rules

A mod found breaking these rules may still be approved. You should expect review feedback. Repeated abuse can become a rejection.

### `other-semicolons`

Semicolons are fine for a tiny guard clause. They are not fine as a way to hide many statements on one line.

Bad:

```lua
local a = readA(); local b = readB(); local c = readC(); print(a, b, c)
```

Good:

```lua
local something: Thing? = notsure.new(); if not something then return end
```

Better when it grows:

```lua
local something: Thing? = notsure.new()
if not something then
    return
end
```

### `other-nice-codebase-bro`

Do not put a large mod into `bootstrap.luau`.

It is called bootstrap because it should start the mod. It should not become the whole mod.

Small mods are fine. If the file is around 500 to 600 lines or more, consider modules.

Bad:

```lua
local settings = {}
local ui = {}
local net = {}
local renderer = {}
```

Good:

```lua
local Settings = require("./Settings")
local Ui = require("./Ui")
local Renderer = require("./Renderer")

Settings.load()
Ui.mount()
Renderer.start()
```

### `other-styling`

Use any readable Lua style. Keep it consistent.

Lua has no single official style guide. The community often follows patterns from LuaRocks, Olivine Labs, and StyLua.

Mixing styles like a blender makes review slower.

Bad:

```lua
local value=1
local function DoThing ()
  return value
end
```

Good:

```lua
local value = 1

local function doThing()
    return value
end
```

### `other-remake`

Do not remake an existing Index mod in Luau with the same purpose and no meaningful changes.

Users do not need the same mod twice. Contribute to the original mod, ask permission, or add something meaningfully new.

Bad:

```lua
local name = "Same features, same UI, new name"
```

Good:

```lua
local name = "New feature set, clear credit, and permission"
```

Look, I can't come up with a funny example, but you get the point.

## Non rules

These are allowed when done with care.

1. The whole mod is just a small `bootstrap.luau` file.
2. One line guards that stay readable.
3. A stable `_G["your.modid"]` API table for sharing with other mods.
4. Performance heavy features that are meaningful, optional, and optimized.
5. A remake with permission, credit, and real new value.

## Related

1. [Geode SDK Mod Guidelines](https://docs.geode-sdk.org/mods/guidelines/)
2. [Globals](reference/lua/globals.md)
3. [Sharing APIs between mods](reference/lua/sharing-apis.md)
4. [Modules](reference/lua/modules.md)
5. [Tasks and time](reference/lua/tasks.md)
6. [gd3d](reference/lua/gd3d.md)
7. [Limits and errors](reference/cpp/limits-and-errors.md)
