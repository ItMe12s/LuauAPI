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
- `null` (returns `nil` in Luau)

A missing key returns `nil` only. A stored JSON `null` also returns `nil` only.
Recoverable read or conversion failures return `nil` and an error message.
See [globals](globals.md) Error shapes.

## setSavedValue

```lua
geode.Mod.setSavedValue(key: string, value: any) -> (boolean?, string?)
```

Writes a JSON value to your mod save data. Returns `true` on success.
Returns `nil` and an error message when the value cannot be converted or stored.

## getSettingValue

```lua
geode.Mod.getSettingValue(key: string) -> any?
```

Reads a mod setting from `mod.json`.
Returns the set value, or the `mod.json` default when the key has not been touched,
and `nil` when the key is missing or the read fails.
This always returns exactly one value.

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
The listener stays active until the runtime shuts down.

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
Use it as the prefix when you call `:setID()` on nodes you create. See [game objects](game-objects.md).

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
See [Limits and errors](../cpp/limits-and-errors.md) for caps and error strings.

## Example

```lua
geode.Mod.setSavedValue("count", 3)
print(geode.Mod.getSavedValue("count"))

geode.Mod.setSavedValue("best", { level = 12, stars = 40 })
local saved = geode.Mod.getSavedValue("best")
if saved then
    print(saved.level, saved.stars)
end

print(geode.Mod.getSettingValue("my-toggle"))

geode.Mod.listenForSettingChanges("my-toggle", function(value)
    print("my-toggle is now", value)
end)

geode.Mod.listenForAllSettingChanges(function(key, value)
    print(key .. " changed to", value)
end)
```

## Related

- [fs](fs.md)
- [json](json.md)
- [globals](globals.md)
- [Limits and errors](../cpp/limits-and-errors.md)
- [Getting started](../../getting-started/overview.md)

## Source

- `src/bindings/geode/GeodeModBinding.cpp`
- `tools/luau_codegen/extra_bindings/mod.dluau`
