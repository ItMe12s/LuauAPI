# json

## Summary

`geode.json` converts between JSON text and Luau values,
using the same JSON model as `geode.Mod.getSavedValue` and `setSavedValue`.

JSON types map to Lua as boolean, number, string, `nil` (null), array table, and object table.
Nesting is limited to 32 levels. Past that limit, tuple APIs return `nil` and an error message,
and `dump` raises a Lua error instead. Unsupported Luau types serialize as JSON `null`.

## parse

```lua
geode.json.parse(text: string) -> (any?, string?)
```

Parses a JSON string into a Luau value.
On success returns the value. On failure returns `nil` and an error message.
Input over 8 MiB is rejected with `nil, "json exceeds maximum size"`.

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

Serializes a Luau value to a JSON string.

- An array-like table (sequential integer keys from 1) becomes a JSON array.
- Any other table becomes a JSON object, keeping only string keys.
- `indent` controls formatting. The default is compact.
  A positive number indents with that many spaces, and `-1` indents with tabs.

```lua
geode.json.dump({ x = true })   -- '{"x":true}'
geode.json.dump({ 1, 2, 3 }, 2) -- pretty-printed array, 2-space indent
```

## Related

- [mod](mod.md)
- [fs](fs.md)
- [Globals](globals.md)

## Source

- `src/bindings/geode/GeodeJsonBinding.cpp`
- `src/bindings/geode/JsonConvert.hpp`
- `tools/luau_codegen/extra_bindings/json.dluau`
