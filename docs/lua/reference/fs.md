# Reference: fs

## Summary

This page lists `geode.fs`. It reads and writes files inside the running mod's own directories.

Every call takes a **root** as its first argument. The root selects one of the mod's
directories. Access is sandboxed to that directory. A `path` that escapes the root
(for example with `..`) or an absolute `path` is rejected.

| Root | Directory | Access |
| --- | --- | --- |
| `"save"` | `getSaveDir()` | read + write |
| `"config"` | `getConfigDir()` | read + write |
| `"persistent"` | `getPersistentDir()` | read + write |
| `"resources"` | `getResourcesDir()` | read only |

Passing an unknown root raises a Lua error. Recoverable failures (missing file, escaped
path, write to a read-only root) return `nil` and an error message, so you can handle them
without `pcall`.

## read

```lua
geode.fs.read(root: FsRoot, path: string) -> (string?, string?)
```

Reads a file's contents. Returns the contents, or `nil` and an error message.
Reads are capped at 32 MB.

## write

```lua
geode.fs.write(root: FsRoot, path: string, data: string) -> (boolean?, string?)
```

Writes `data` to a file, creating parent directories as needed. Returns `true`, or `nil`
and an error message. Writing to the read-only `resources` root fails.

## exists

```lua
geode.fs.exists(root: FsRoot, path: string) -> (boolean?, string?)
```

Returns whether a file or directory exists at `path`,
or `nil` and an error message when the path escapes the root.

## list

```lua
geode.fs.list(root: FsRoot, path: string) -> ({ string }?, string?)
```

Lists the immediate entries of a directory (file and directory names, not full paths, not
recursive). Returns an array table, or `nil` and an error message.

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

Removes a single file or empty directory (never recursive). Returns `true`, or `nil` and an
error message. Fails on the read-only `resources` root.

## Example

```lua
local data = geode.json.dump({ count = 3 })
assert(geode.fs.write("save", "state.json", data))

if geode.fs.exists("save", "state.json") then
    local text = geode.fs.read("save", "state.json")
    print(geode.json.parse(text).count) -- 3
end

for _, name in geode.fs.list("save", ".") do
    print(name)
end

geode.fs.remove("save", "state.json")
```

## Related

- [Reference: json](json.md)
- [Reference: mod](mod.md)
- [Globals](globals.md)

## Source

- `src/lua/bindings/geode/GeodeFsBinding.cpp`
- `src/lua/module/PathSandbox.hpp`
