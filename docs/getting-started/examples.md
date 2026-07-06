# Examples

## Summary

Small runnable scripts for common tasks. Each block stands on its own.
Full signatures live in the [globals](../reference/lua/globals.md).

## Load a module with require

Full API: [modules](../reference/lua/modules.md).

```lua
-- Math.luau
local M = {}
function M.add(a, b)
    return a + b
end
return M
```

```lua
local Math = require("./Math")
print(Math.add(2, 3))
```

## Hook a game function

Full API: [hooks](../reference/lua/hooks.md).
Demo: [mod/demo/demo_helloworld.luau](../../mod/demo/demo_helloworld.luau).

```lua
geode.hook("geode.gd.MenuLayer:init/0", {
    after = function(self, result)
        print("MenuLayer opened")
        return result
    end,
})
```

## Read and write encrypted stat fields

Full API: [game objects](../reference/lua/game-objects.md) (Encrypted stat fields).
Demo: [mod/demo/demo_seedvalue.luau](../../mod/demo/demo_seedvalue.luau).

SeedValue fields such as `m_levelID` and `m_attempts` bind as plain Lua numbers.
Run the demo from the in-game executor or your mod's `runFile` call.

```lua
local level = nil

geode.hook("geode.gd.LevelInfoLayer:init/2", {
    after = function(self, _, _, result)
        if result then
            level = self.m_level
        end
        return result
    end,
})

imgui.onDraw(function()
    imgui.window("Stats", function()
        if not level then
            return
        end
        imgui.text(string.format("attempts: %d", level.m_attempts))
        if imgui.button("+1") then
            level.m_attempts += 1
        end
    end)
end)
```

## Schedule work with task

Full API: [tasks and time](../reference/lua/tasks.md).

```lua
task.spawn(function()
    print("runs now")
end)

task.defer(function()
    print("runs next tick")
end)

local handle = task.delay(1.0, function()
    print("runs after one second")
end)

local ticks = 0
local everyHandle
everyHandle = task.every(0.5, function()
    ticks += 1
    print("tick", ticks)
    if ticks >= 5 then
        everyHandle:cancel()
    end
end)
```

## Save data with fs, json, and Mod

Full API: [fs](../reference/lua/fs.md), [json](../reference/lua/json.md), [mod](../reference/lua/mod.md).

```lua
local raw, err = geode.fs.read("save", "stats.json")
local stats = raw and geode.json.parse(raw) or { wins = 0 }

stats.wins += 1
geode.Mod.setSavedValue("highScore", stats.wins)

local ok, writeErr = geode.fs.write("save", "stats.json", geode.json.dump(stats))
if not ok then
    print(writeErr)
end
```

## Share an API on _G

Full API: [sharing APIs between mods](../reference/lua/sharing-apis.md).

Publish from the provider mod. Read with `_G["other.mod.id"]` from consumers.
Use `task` polling when load order is unknown.

## Fetch over the web

Full API: [web](../reference/lua/web.md).

## WebSocket client and server

Full API: [websocket](../reference/lua/websocket.md).

## Draw a debug overlay

Full API: [imgui](../reference/lua/imgui.md).
Demo: [mod/demo/demo_modmenu.luau](../../mod/demo/demo_modmenu.luau).

```lua
local state = { enabled = false }

imgui.onDraw(function()
    imgui.window("Demo", function()
        imgui.text("Hello from Luau")
        if imgui.button("Click me") then
            print("clicked")
        end
        state.enabled = imgui.checkbox("Enable", state.enabled)
    end)
end)
```

## Spin a 3D mesh in a viewport

Full API: [gd3d](../reference/lua/gd3d.md).
Demo: [mod/demo/demo_viewport.luau](../../mod/demo/demo_viewport.luau) loads `resources/test_donut.glb`.

```lua
local Transform = gd3d.Transform
local cc2d = geode.cocos2d
local modId = geode.Mod.getID()

local mesh, err = gd3d.gltf.loadMesh("resources", "test_donut.glb")
if not mesh then
    print(err)
    return
end

local vp = gd3d.ViewportFrame.new(200, 200)
if not vp then
    print("failed to create ViewportFrame")
    return
end

vp:setID(modId .. "/spinning-mesh")
vp:setCamera(Transform.new({ x = 0, y = 1, z = 3 }, { x = 0, y = 0, z = 0 }), 90, 0.1, 100)

local id = vp:addMesh(mesh, Transform.new())
local angle = 0

task.every(1 / 60, function()
    angle += 0.02
    vp:setInstanceTransform(id, Transform.fromEuler(0.4, angle, 0))
end)

geode.hook("geode.gd.MenuLayer:init/0", {
    after = function(self, result)
        local director = cc2d.CCDirector.sharedDirector()
        if not director then
            return result
        end

        local winSize = director:getWinSize()
        vp:setPosition({ x = winSize.width / 2, y = winSize.height / 2 })
        self:addChild(vp)
        return result
    end,
})
```

## Next

- [LuauAPI mod guidelines](../mod_guidelines.md)
- [globals](../reference/lua/globals.md)
- [hooks](../reference/lua/hooks.md)

## Related

- [Getting started](overview.md)
- [LuauAPI mod guidelines](../mod_guidelines.md)
- [globals](../reference/lua/globals.md)
- [modules](../reference/lua/modules.md)
- [sharing APIs between mods](../reference/lua/sharing-apis.md)
- [hooks](../reference/lua/hooks.md)
- [game objects](../reference/lua/game-objects.md)
- [tasks and time](../reference/lua/tasks.md)
- [imgui](../reference/lua/imgui.md)
- [gd3d](../reference/lua/gd3d.md)

## Source

- `src/core/Runtime.cpp`
- `mod/demo/demo_helloworld.luau`
- `mod/demo/demo_seedvalue.luau`
- `mod/demo/demo_modmenu.luau`
- `mod/demo/demo_viewport.luau`
- `src/bindings/task/TaskBinding.cpp`
- `src/bindings/geode/web/GeodeWebCore.cpp`
- `src/bindings/websocket/WebSocketBinding.cpp`
- `src/bindings/imgui/ImGuiCore.cpp`
