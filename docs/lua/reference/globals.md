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

## require

```lua
require(path: string) -> any
```

Loads a sibling Luau module and returns its single value. The module system is flat and sandboxed. See [Modules and require](../guide/modules-and-require.md) for the full rules.

```lua
local Helper = require("./Helper")
```

## _G

```lua
_G: { [string]: any }
```

The shared global table for every script in the runtime. There is one runtime and one global table, so values you set on `_G` are visible to other scripts and to other mods. This is the channel for cross mod APIs. Read shared values through `_G[key]` rather than as a bare global name. See [Sharing APIs between mods](../guide/sharing-apis-between-mods.md) for the full convention.

## Other globals

The standard Luau libraries are open, including `string`, `table`, and `math`. The `task`, `time`, and `geode` globals are documented on their own pages.

- [task](task.md)
- [time](time.md)
- [Hooks](hooks.md)

## Source

- `src/lua/runtime/Runtime.cpp`
- `src/lua/module/Requirer.cpp`
