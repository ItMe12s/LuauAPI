# Modules and require

## Summary

`require` loads another Luau file as a module.
The module system is flat and sandboxed, so you can only load sibling files inside the same resources root.

## How to use it

A module file returns one value, and the caller receives that value.

```lua
-- Helper.luau
local M = {}
function M.add(a, b)
    return a + b
end
return M
```

```lua
-- main script
local Helper = require("./Helper")
print(Helper.add(1, 2))
```

## The rules

The rules are strict by design, because they keep loading inside the resources root.

- You can only require from a script file. The script must be a loaded chunk, which means its name starts with `@`.
- Module names are flat. They are a single file name with no folders in the path.
- `..` is not allowed, and escaping the root is not allowed.
- Aliases are not supported. A path such as `@alias/name` is rejected.
- The extension must be `.luau`, or absent. When you leave it off, `.luau` is added for you.
- A module must return exactly one value.
- A module must not yield.

## Size and caching

A module uses the same size limit as a script. See [Limits and errors](../../cpp/limits-and-errors.md) for all limits.
Compiled modules share the bytecode cache with scripts. The cache key is the path, size, modify time, and content hash.

## What happens on load

When you require a module, the runtime resolves the path inside the root,
reads the file, compiles it to bytecode, and runs it on a sandboxed thread.
The single returned value is handed back to the caller.

## Limits and notes

Modules load with the default script deadline. See [Limits and errors](../../cpp/limits-and-errors.md) for all limits.
The rules above are enforced at load time. Breaking any of them returns an error.

## Related

- [Writing scripts](writing-scripts.md)
- [Sharing APIs between mods](sharing-apis-between-mods.md)
- [Module system internals](../../dev/module-system.md)

## Source

- `src/lua/module/Requirer.cpp`
- `src/lua/module/RequirePath.hpp`
- `src/lua/module/PathRules.hpp`
- `src/lua/Config.hpp`
