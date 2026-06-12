# globals

## Summary

The globally available functions and values in every script are:

- `print`
- `loadstring`
- `require`
- `_G`

This page covers signatures for those globals plus error shapes.

## Script basics

Runtime rules for scripts live in [Getting started Key concepts](../../getting-started/overview.md).
Caps and error strings live in [Limits and errors](../cpp/limits-and-errors.md).

## Error shapes

Some APIs return `nil` and an error string instead of throwing.
Examples include `loadstring`, `fs` reads, `json` parse, `keybind.fromString`, and `websocket.serve`.
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

`loadstring` is always available, can compile any string, and ignores the `require` sandbox.
Treat it as trusted-code execution. Calling the returned function does not start a new script deadline.
It inherits an existing guarded call path, such as code already running inside a `task` callback.

## require

```lua
require(path: string) -> any
```

Loads a sibling Luau module and returns its single value.
See [Modules](modules.md) for the full rules.
See [Examples](../../getting-started/examples.md).

## _G

```lua
_G: { [string]: any }
```

The shared global table for every script in the runtime.
There is one runtime and one global table, so values you set on `_G` are visible to other scripts and to other mods.
Read shared values through `_G[key]` rather than as a bare global name. See [Sharing APIs between mods](sharing-apis.md).

## Other globals

Standard Luau libraries and LuauAPI namespaces (`task`, `geode`, `imgui`, `gd3d`, and others) are listed in [Type stubs](type-stubs.md).

## Limits

`loadstring` rejects sources over the script size cap.

See [Limits and errors](../cpp/limits-and-errors.md).

## Related

- [Modules](modules.md)
- [Sharing APIs between mods](sharing-apis.md)
- [Type stubs](type-stubs.md)
- [Getting started overview](../../getting-started/overview.md)

## Source

- `src/core/Runtime.cpp`
- `src/require/Requirer.cpp`
- `src/core/Config.hpp`
