# Reference: mod

## Summary

This page lists `geode.Mod`. It reads the host mod metadata, paths, saved values, and settings.

## getSavedValue

```lua
geode.Mod.getSavedValue(key: string) -> any?
```

Reads a value from the mod save file. Values are JSON types such as boolean, number, string, table, or array.
Returns `nil` when the key is missing or the read fails.

## setSavedValue

```lua
geode.Mod.setSavedValue(key: string, value: any) -> ()
```

Writes a JSON value to the mod save file. Tables and arrays are stored as JSON objects and arrays.

## getSettingValue

```lua
geode.Mod.getSettingValue(key: string) -> any?
```

Reads a mod setting from `mod.json`. Returns `nil` when the key is missing or the read fails.

## hasSetting

```lua
geode.Mod.hasSetting(key: string) -> boolean
```

Returns true when the setting key exists in `mod.json`.

## getID

```lua
geode.Mod.getID() -> string
```

Returns the mod id, for example `my.mod.id`.

## getName

```lua
geode.Mod.getName() -> string
```

Returns the mod display name.

## getVersion

```lua
geode.Mod.getVersion() -> string
```

Returns the mod version string.

## getResourcesDir

```lua
geode.Mod.getResourcesDir() -> string
```

Returns the path to the mod resources folder. Script files live here.

## getSaveDir

```lua
geode.Mod.getSaveDir() -> string
```

Returns the path to the mod save folder.

## getConfigDir

```lua
geode.Mod.getConfigDir() -> string
```

Returns the path to the mod config folder.

## Example

```lua
geode.Mod.setSavedValue("count", 3)
print(geode.Mod.getSavedValue("count"))

if geode.Mod.hasSetting("enable-executor") then
    print(geode.Mod.getSettingValue("enable-executor"))
end

print(geode.Mod.getResourcesDir())
```

## Related

- [Modules and require](../guide/modules-and-require.md)
- [Globals](globals.md)

## Source

- `src/lua/bindings/geode/GeodeModBinding.cpp`
