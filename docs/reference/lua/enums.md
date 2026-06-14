# enums

## Summary

How GD and Geode enums appear in scripts, stubs, and bindings.

## Constant tables

GD enums live on `geode.gd`. Geode enums live on `geode`.
Each enum is a table of integer members.

```lua
local gd = geode.gd

if levelType ~= gd.GJLevelType.SearchResult
    and levelType ~= gd.GJLevelType.Saved then
    return
end
```

Cocos `enumKeyCodes` is similar but lives on `geode.cocos`. See [cocos](cocos.md).

## Runtime values

Bindings pass enum arguments and returns as plain numbers.
Members like `gd.GJLevelType.Saved` are the same integers.
Use `==` and `~=` like any other number compare.

There is no separate enum type at runtime.

## Type stubs

Each bound enum has two shapes in `types/geode.d.luau`:

- `export type GJLevelType = number` for method args and returns
- `export type GJLevelTypeNamespace = { Saved: number, SearchResult: number, ... }` for the constant table

The type checker types every enum as `number`, so it does not block compares across different enum names.
Only the integer value matters.

Prefer named constants over magic numbers.

## Related

- [game objects](game-objects.md)
- [cocos](cocos.md)
- [Callbacks](callbacks.md)
- [Type stubs](type-stubs.md)
- [Codegen](../../contributor/codegen/codegen.md)

## Source

- `tools/luau_codegen/emit/bindings/geode_enums.py`
- `tools/luau_codegen/emit/luau_types/enums.py`
- `tools/luau_codegen/convert/marshalling.py`
