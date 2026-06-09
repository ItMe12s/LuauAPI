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

## Related

- [Globals](../reference/lua/globals.md)
- [Hooks](../reference/lua/hooks.md)
- [Tasks and time](../reference/lua/tasks.md)
- [web](../reference/lua/web.md)
- [imgui](../reference/lua/imgui.md)

## Source

- `src/lua/runtime/Runtime.cpp`
- `src/lua/bindings/task/TaskBinding.cpp`
- `src/lua/bindings/geode/GeodeWebBinding.cpp`
- `src/lua/bindings/imgui/ImGuiBinding.cpp`
