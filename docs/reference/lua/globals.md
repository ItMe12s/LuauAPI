# globals

## Summary

Core script globals in every Luau chunk:

- `print`
- `loadstring`
- `require`
- `_G`

LuauAPI also exposes namespaces such as `task`, `geode`, `imgui`, and `gd3d`.
See Other globals below.

This page covers signatures for the core globals plus error shapes.

## Script basics

Runtime rules live in [Getting started](../../getting-started/overview.md).
Caps and error strings live in [Limits and errors](../cpp/limits-and-errors.md).

## All modules

| Group | Page | Role |
| --- | --- | --- |
| Core | [hooks](hooks.md) | Hook game functions |
| Core | [modules](modules.md) | Sandboxed `require` |
| Core | [sharing APIs between mods](sharing-apis.md) | `_G` mod APIs |
| Core | [tasks and time](tasks.md) | `task` and `time` |
| Core | [callbacks](callbacks.md) | C++ callback lifetime |
| Core | [delegates](delegates.md) | Virtual interface tables |
| Game | [enums](enums.md) | GD and Geode enum constants |
| Game | [game objects](game-objects.md) | Cocos and GD objects |
| Game | [cocos](cocos.md) | Node and color helpers |
| Game | [ColorProvider](color-provider.md) | Theme colors |
| Game | [Keybind](keybind.md) | Keybind strings |
| Game | [Keyboard input](keyboard-input.md) | Keyboard events |
| UI | [UI and layouts](ui.md) | Cocos UI factories |
| UI | [imgui](imgui.md) | Dear ImGui overlay |
| UI | [gd3d](gd3d.md) | 3D viewport rendering |
| IO | [mod](mod.md) | Saved values and settings |
| IO | [fs](fs.md) | Sandboxed filesystem |
| IO | [json](json.md) | JSON parse and dump |
| Network | [web](web.md) | HTTP client |
| Network | [websocket](websocket.md) | WebSocket client and server |
| Utils | [geode.utils](utils.md) | Utils namespace index |
| Utils | [clipboard](utils.md) | System clipboard |
| Utils | [string](utils.md) | String helpers |
| Utils | [random](utils.md) | Random strings and UUIDs |
| Utils | [game](game.md) | Game process control |
| Utils | [base64](base64.md) | Base64 encode and decode |
| Utils | [permission](permission.md) | OS permissions |
| Utils | [VersionInfo](version-info.md) | Version parsing |
| Meta | [type stubs](type-stubs.md) | `geode.d.luau` reference |

## Error shapes

Some APIs return `nil` and an error string instead of throwing.
Examples include `loadstring`, `fs` reads, `json` parse, `geode.Keybind.fromString`, and `websocket.serve`.
Other APIs raise on failure, such as `json.dump` and invalid hook targets.
Check each function's signature for which shape it uses.

## print

```lua
print(...any) -> ()
```

Writes a single line to the Geode log. Each argument is converted to text and joined with a tab.

```lua
print("value", 1, true)
```

## loadstring

```lua
loadstring(source: string, chunkName: string?) -> (((...any) -> ...any)?, string?)
```

Compiles a Luau source string and returns a callable function. It does not run the chunk.
On syntax or load failure it returns `nil` plus an error string instead of throwing.

```lua
local fn, err = loadstring("return 1 + 1")
if not fn then
    print("compile failed:", err)
    return
end
print(fn())
```

`chunkName` is used in diagnostics. If omitted, LuauAPI uses `=loadstring`.
Sources over the script size cap return `nil` and an error message.
If the runtime is not ready, it returns `nil, "luau runtime not ready"`.

`loadstring` is always available, can compile any valid Luau source within the size cap, and ignores the `require` sandbox.
Treat it as trusted-code execution. Calling the returned function does not start a new script deadline.
It inherits an existing guarded call path, such as code already running inside a `task` callback.

## require

```lua
require(path: string) -> any
```

Loads a sibling Luau module and returns its single value.
See [modules](modules.md) for the full rules.
See [Examples](../../getting-started/examples.md).

## _G

```lua
_G: { [string]: any }
```

The shared global table for every script in the runtime.
There is one runtime and one global table, so values you set on `_G` are visible to other scripts and to other mods.
Read shared values through `_G[key]` rather than as a bare global name. See [sharing APIs between mods](sharing-apis.md).

## Other globals

Standard Luau libraries and LuauAPI namespaces (`task`, `geode`, `imgui`, `gd3d`, and others) are listed in [type stubs](type-stubs.md).
Hook helpers such as `geode.skip` are documented in [hooks](hooks.md).

## Limits

`loadstring` rejects sources over the script size cap.

See [Limits and errors](../cpp/limits-and-errors.md).

## Related

- [Getting started](../../getting-started/overview.md)
- [Examples](../../getting-started/examples.md)
- [modules](modules.md)
- [sharing APIs between mods](sharing-apis.md)
- [geode.utils](utils.md)
- [type stubs](type-stubs.md)
- [Limits and errors](../cpp/limits-and-errors.md)

## Source

- `src/core/Runtime.cpp`
- `src/require/Requirer.cpp`
- `src/core/Config.hpp`
