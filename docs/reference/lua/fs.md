# fs

## Summary

`geode.fs` reads and writes files inside the running mod's own directories.
Every call takes a root as its first argument.
The root selects one of the mod's directories, and access is sandboxed to it.
A path that escapes the root (for example with `..`) or an absolute path is rejected.

| Root | Directory | Access |
| --- | --- | --- |
| `"save"` | `getSaveDir()` | read + write |
| `"config"` | `getConfigDir()` | read + write |
| `"persistent"` | `getPersistentDir()` | read + write |
| `"resources"` | `getResourcesDir()` | read only |

An unknown root raises a Lua error.

Recoverable failures return `nil` and an error string. See [globals](globals.md) Error shapes.

### Symlinks

The sandbox blocks `..` and absolute paths and resolves the path inside the root.
It does not block symlinks. A symlink already inside a root can still point outside it.
LuauAPI never creates symlinks, so this only matters if something else placed one.
The sandbox is not a defense against a hostile local filesystem.

## read

```lua
geode.fs.read(root: FsRoot, path: string) -> (string?, string?)
```

Reads a file's contents. Returns the contents, or `nil` and an error message.

## write

```lua
geode.fs.write(root: FsRoot, path: string, data: string) -> (boolean?, string?)
```

Writes `data` to a file, creating parent directories as needed. Returns `true`, or `nil` and an error message.
Writing to the read-only `resources` root fails.

## exists

```lua
geode.fs.exists(root: FsRoot, path: string) -> (boolean?, string?)
```

Returns `true` when a file or directory exists, `false` when missing.
Returns `nil` and an error message when the path escapes the root or the filesystem fails.

## list

```lua
geode.fs.list(root: FsRoot, path: string) -> ({ string }?, string?)
```

Lists the immediate entries of a directory (names, not full paths, not recursive).
Returns an array table, or `nil` and an error message.
Listings are capped.

## mkdir

```lua
geode.fs.mkdir(root: FsRoot, path: string) -> (boolean?, string?)
```

Creates a directory and any missing parents. Returns `true`, or `nil` and an error message.
Fails on the read-only `resources` root.

## remove

```lua
geode.fs.remove(root: FsRoot, path: string) -> (boolean?, string?)
```

Removes a single file or empty directory (never recursive).
Returns `true`, or `nil` and an error message. Fails on the read-only `resources` root.

## Limits

Reads, writes, and directory listings are capped.

See [Limits and errors](../cpp/limits-and-errors.md).

## Example

```lua
local data = geode.json.dump({ count = 3 })
assert(geode.fs.write("save", "state.json", data))

if geode.fs.exists("save", "state.json") then
    local text = geode.fs.read("save", "state.json")
    print(geode.json.parse(text).count) -- 3
end
```

## Related

- [Getting started](../../getting-started/overview.md)
- [Examples](../../getting-started/examples.md)
- [json](json.md)
- [mod](mod.md)
- [globals](globals.md)
- [Limits and errors](../cpp/limits-and-errors.md)

## Source

- `src/bindings/geode/GeodeFsBinding.cpp`
- `src/require/PathSandbox.hpp`
- `tools/luau_codegen/extra_bindings/fs.dluau`
