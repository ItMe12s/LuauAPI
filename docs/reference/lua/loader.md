# loader

## Summary

`geode.Loader` reads Geode loader state for installed mods.
Use it when scripts need mod ids, names, versions, or load status.

## getAllMods

```lua
geode.Loader.getAllMods() -> { LoaderModInfo }
```

Returns one table per installed mod. Each entry has:

- `id`: mod id string
- `name`: display name
- `developer`: comma-separated developer names
- `version`: version string
- `enabled`: true when the mod is loaded
- `shouldLoad`: true when Geode plans to load the mod this session

Order matches Geode loader order.

## Example

```lua
for _, mod in geode.Loader.getAllMods() do
    print(mod.id, mod.name, mod.version)
end
```

## Related

- [mod](mod.md)
- [globals](globals.md)
- [type stubs](type-stubs.md)
- [Getting started](../../getting-started/overview.md)

## Source

- `src/bindings/geode/GeodeLoaderBinding.cpp`
- `tools/luau_codegen/extra_bindings/loader.dluau`
