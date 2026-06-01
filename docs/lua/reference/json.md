# Reference: json

## Summary

This page lists `geode.json`. It converts between JSON text and Lua values,
using the same JSON model as `geode.Mod.getSavedValue`/`setSavedValue`.

JSON types map to Lua as: boolean, number, string, `nil` (null), array table, and object table.
Conversion is bounded to 32 levels of nesting, anything deeper is treated as null.

## parse

```lua
geode.json.parse(text: string) -> (any?, string?)
```

Parses a JSON string into a Lua value.
On success returns the value. On failure returns `nil` and an error message.

```lua
local value, err = geode.json.parse('{"a":[1,2,3]}')
if not value then
    print("bad json: " .. err)
else
    print(value.a[2]) -- 2
end
```

## dump

```lua
geode.json.dump(value: any, indent: number?) -> string
```

Serializes a Lua value to a JSON string.

- An array-like table (sequential integer keys from 1) becomes a JSON array.
- Any other table becomes a JSON object, only string keys are kept.
- `indent` controls formatting. The default is compact (no whitespace). A positive number
  indents with that many spaces, `-1` indents with tabs.

```lua
geode.json.dump({ x = true })   -- '{"x":true}'
geode.json.dump({ 1, 2, 3 }, 2) -- pretty-printed array, 2-space indent
```

## Related

- [Reference: mod](mod.md) saved values and settings use the same JSON model
- [Reference: fs](fs.md) read and write JSON files on disk
- [Globals](globals.md)

## Source

- `src/lua/bindings/geode/GeodeJsonBinding.cpp`
- `src/lua/bindings/geode/JsonConvert.hpp`
