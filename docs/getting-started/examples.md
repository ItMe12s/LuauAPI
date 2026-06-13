# Examples

## Summary

A few small scripts that show the common things a script does. Each one is complete and runnable on its own.
For full signatures, see the [Lua reference](../reference/lua/globals.md).

## Log and return a module

A file run with `runFile` does not need to return anything. A file loaded with `require` must return exactly one value.

```lua
-- Helper.luau
local M = {}
function M.greet(name)
    print("hello", name)
end
return M
```

## Hook a game function

`geode.hook` runs your code before or after a method. The target id is `namespace.Class:method/argCount`.

```lua
geode.hook("geode.gd.MenuLayer:init/0", {
    after = function(self, result)
        print("MenuLayer opened")
        return result
    end,
})
```

## Schedule work with task

`task.delay`, `task.every`, and `task.defer` return a handle you can cancel.

```lua
local ticks = 0
local handle
handle = task.every(0.5, function()
    ticks += 1
    print("tick", ticks)
    if ticks >= 5 then
        handle:cancel()
    end
end)
```

## Fetch over the web

Web requests run on Geode's worker and call back on the main thread.

```lua
geode.utils.web.get("https://api.example.com/data", function(response, err)
    if err then
        print(err)
        return
    end
    local data, jsonErr = response:json()
    if data then
        print(data.version)
    end
end)
```

## WebSocket client

WebSocket I/O runs on background threads and delivers events to Lua on the main thread.
Register callbacks right after `connect` so you do not miss early events.

```lua
local ws = websocket.connect("wss://echo.websocket.org")
ws:onOpen(function()
    ws:send("hello")
end):onMessage(function(data, isBinary)
    print("received:", data)
end):onClose(function(code, reason, remote)
    print("closed:", code, reason)
end):onError(function(message)
    print("error:", message)
end)
```

## WebSocket server

`websocket.serve` starts a local server. It binds to loopback (`127.0.0.1`) by default.
Only pass `host = "0.0.0.0"` when you intend to expose the port on your LAN.

```lua
local server, err = websocket.serve(7777)
if not server then
    print(err)
    return
end

server:onClientConnect(function(peer)
    peer:send("welcome")
end):onMessage(function(peer, data)
    print("from client:", data)
end)
```

Pair with a client on the same machine:

```lua
local ws = websocket.connect("ws://127.0.0.1:7777")
ws:onMessage(function(data)
    print("from server:", data)
end)
```

## Draw a debug overlay

`imgui.onDraw` runs every frame. Build windows and widgets inside the callback.

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

Pack a `.glb` or `.gltf` file in your mod resources, load it, and parent a `ViewportFrame` under the menu when it opens.
See [gd3d](../reference/lua/gd3d.md) and [src/scripts/_viewportdemo.luau](../../src/scripts/_viewportdemo.luau) for a longer demo.

```lua
local Transform = gd3d.Transform
local cc2d = geode.cocos2d
local modId = geode.Mod.getID()

local mesh, err = gd3d.gltf.loadMesh("resources", "model.glb")
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

## Related

- [Globals](../reference/lua/globals.md)
- [Hooks](../reference/lua/hooks.md)
- [Tasks and time](../reference/lua/tasks.md)
- [web](../reference/lua/web.md)
- [websocket](../reference/lua/websocket.md)
- [imgui](../reference/lua/imgui.md)
- [gd3d](../reference/lua/gd3d.md)

## Source

- `src/core/Runtime.cpp`
- `src/bindings/task/TaskBinding.cpp`
- `src/bindings/geode/web/GeodeWebBinding.cpp`
- `src/bindings/websocket/WebSocketBinding.cpp`
- `src/bindings/imgui/ImGuiBinding.cpp`
