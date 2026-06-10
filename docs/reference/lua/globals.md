# globals

## Summary

The globally available functions and values in every script are:

- `print`
- `loadstring`
- `require`
- `_G`

This page also covers script basics like logging, error handling, and the time budget.

## Script basics

- Your script runs on the main thread.
- Errors are caught and written to the log. They do not crash the game.
  The host can read the last error after a run.
- Each run has a time budget in milliseconds. Going over raises an error.
  Avoid long loops and blocking calls. Spread heavy work across frames with `task`.
- Memory has a hard cap. Once it is reached, allocation fails.
  See [Limits and errors](../cpp/limits-and-errors.md).

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
Sources over 4 MiB are rejected with `nil, "script exceeds maximum size"`.
If the runtime is not ready, it returns `nil, "luau runtime not ready"`.

`loadstring` is always available, can compile any string, and ignores the `require` sandbox.
Treat it as trusted-code execution. Calling the returned function does not start a new script deadline.
It inherits an existing guarded call path, such as code already running inside a `task` callback.

## require

```lua
require(path: string) -> any
```

Loads a sibling Luau module and returns its single value.
The module system is flat and sandboxed.
See [Modules](modules.md) for the full rules.

```lua
local Helper = require("./Helper")
```

## _G

```lua
_G: { [string]: any }
```

The shared global table for every script in the runtime.
There is one runtime and one global table, so values you set on `_G` are visible to other scripts and to other mods.
Read shared values through `_G[key]` rather than as a bare global name. See [Sharing APIs between mods](sharing-apis.md).

## Other globals

The following standard Luau libraries and globals are available:

- `string`
- `table`
- `math`
- `task` (see its own page)
- `time` (see its own page)
- `geode` (see its own page)

## Related

- [Modules](modules.md)
- [Sharing APIs between mods](sharing-apis.md)
- [Tasks and time](tasks.md)
- [Limits and errors](../cpp/limits-and-errors.md)

## Source

- `src/core/Runtime.cpp`
- `src/require/Requirer.cpp`
- `src/core/Config.hpp`
