# Reference: mod

## Summary

This page lists `geode.Mod`. It reads the host mod metadata, paths, saved values, and settings. It can also listen for setting changes.

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

## listenForSettingChanges

```lua
geode.Mod.listenForSettingChanges(key: string, callback: (value: any) -> ())
```

Calls your function whenever the setting with this key changes. The function gets the new value.
The value is a JSON type such as boolean, number, string, or table. The listener stays active until the script unloads.

## listenForAllSettingChanges

```lua
geode.Mod.listenForAllSettingChanges(callback: (key: string, value: any) -> ())
```

Calls your function whenever any setting for this mod changes.
The function gets the setting key and the new value. The listener stays active until the script unloads.

## getID

```lua
geode.Mod.getID() -> string
```

Returns the mod id, for example `my.mod.id`.

Use this as the prefix when you call `:setID()` on nodes you create.
See [Using game objects](../guide/using-game-objects.md).

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

geode.Mod.listenForSettingChanges("my-toggle", function(value)
    print("my-toggle is now", value)
end)

print(geode.Mod.getResourcesDir())
```

## Related

- [Modules and require](../guide/modules-and-require.md)
- [Globals](globals.md)

## Source

- `src/lua/bindings/geode/GeodeModBinding.cpp`
