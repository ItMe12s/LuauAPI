# mod

## Summary

`geode.Mod` reads the host mod metadata, paths, saved values, and settings.
It can also listen for setting changes.

## getSavedValue

```lua
geode.Mod.getSavedValue(key: string) -> (any?, string?)
```

Reads a value from the mod save file.

Values are JSON types:

- boolean
- number
- string
- table
- array

Returns `nil` when the key is missing or the read fails.
Returns `nil` and an error message when the stored value is too deeply nested.

## setSavedValue

```lua
geode.Mod.setSavedValue(key: string, value: any) -> (boolean?, string?)
```

Writes a JSON value to the mod save file.
Returns `true` on success. Returns `nil` and an error message when conversion or save fails.

## getSettingValue

```lua
geode.Mod.getSettingValue(key: string) -> (any?, string?)
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

Calls your function whenever the setting with this key changes.
The listener stays active until the script unloads.

## listenForAllSettingChanges

```lua
geode.Mod.listenForAllSettingChanges(callback: (key: string, value: any) -> ())
```

Calls your function whenever any setting for this mod changes. It gets the key and the new value.

## getID, getName, getVersion

```lua
geode.Mod.getID() -> string
geode.Mod.getName() -> string
geode.Mod.getVersion() -> string
```

`getID` returns the mod id, for example `my.mod.id`.
Use it as the prefix when you call `:setID()` on nodes you create. See [Game objects](game-objects.md).

## getResourcesDir, getSaveDir, getConfigDir, getPersistentDir

```lua
geode.Mod.getResourcesDir() -> string
geode.Mod.getSaveDir() -> string
geode.Mod.getConfigDir() -> string
geode.Mod.getPersistentDir() -> string
```

Return the paths to the mod resources, save, config, and persistent folders. Script files live in resources.

## Limits

Saved values and settings use the JSON model.
Depth and parse size limits match [json](json.md).
Saved values over the cap fail with `json exceeds maximum size` or `json exceeds maximum depth`.
See [Limits and errors](../cpp/limits-and-errors.md) for the full table.

## Example

```lua
geode.Mod.setSavedValue("count", 3)
print(geode.Mod.getSavedValue("count"))

geode.Mod.listenForSettingChanges("my-toggle", function(value)
    print("my-toggle is now", value)
end)
```

## Related

- [Getting started overview](../../getting-started/overview.md)
- [Examples](../../getting-started/examples.md)
- [fs](fs.md)
- [json](json.md)
- [Globals](globals.md)
- [Limits and errors](../cpp/limits-and-errors.md)

## Source

- `src/bindings/geode/GeodeModBinding.cpp`
- `tools/luau_codegen/extra_bindings/mod.dluau`
