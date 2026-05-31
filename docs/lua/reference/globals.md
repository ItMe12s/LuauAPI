# Reference: globals

## Summary

This page lists the global functions available to every script.

## print

```lua
print(...any) -> ()
```

Writes a single line to the Geode log. Each argument is converted to text and joined with a tab. Returns nothing.

```lua
print("value", 1, true)
```

## loadstring

```lua
loadstring(source: string, chunkName: string?) -> (((...any) -> ...any)?, string?)
```

Compiles a Luau source string and returns a callable function. It does not run the chunk.
On syntax or load failure, returns `nil` plus an error string instead of throwing.

```lua
local fn, err = loadstring("return 1 + 1")
if not fn then
    print("compile failed:", err)
    return
end

print(fn())
```

The optional `chunkName` is used in diagnostics and debug output. If omitted, LuauAPI uses `=loadstring`.
Sources over 4 MiB are rejected with `nil, "script exceeds maximum size"`.
If the runtime is not ready, returns `nil, "luau runtime not ready"`.

`loadstring` is always available, can compile any string, and ignores the `require` sandbox.
Calling the returned function from Lua does not start a new script deadline.
It only inherits an existing guarded call path, such as code already running inside a `task` callback.

## require

```lua
require(path: string) -> any
```

Loads a sibling Luau module and returns its single value. The module system is flat and sandboxed.
See [Modules and require](../guide/modules-and-require.md) for the full rules.

```lua
local Helper = require("./Helper")
```

## _G

```lua
_G: { [string]: any }
```

The shared global table for every script in the runtime.
There is one runtime and one global table, so values you set on `_G` are visible to other scripts and to other mods.
This is the channel for cross mod APIs. Read shared values through `_G[key]` rather than as a bare global name.
See [Sharing APIs between mods](../guide/sharing-apis-between-mods.md) for the full convention.

## Other globals

The standard Luau libraries are open, including `string`, `table`, and `math`.
The `task`, `time`, and `geode` globals are documented on their own pages.

- [task](task.md)
- [time](time.md)
- [mod](mod.md)
- [Hooks](hooks.md)

## Source

- `src/lua/runtime/Runtime.cpp`
- `src/lua/module/Requirer.cpp`
