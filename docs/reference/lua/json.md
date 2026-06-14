# json

## Summary

`geode.json` converts between JSON text and Luau values,
using the same JSON model as `geode.Mod.getSavedValue` and `setSavedValue`.

JSON types map to Lua as boolean, number, string, `nil` (null), array table, and object table.
Past depth and size limits, tuple APIs return `nil` and an error message, and `dump` raises.
See [Globals](globals.md) Error shapes.
Unsupported Luau types serialize as JSON `null`.

## parse

```lua
geode.json.parse(text: string) -> (any?, string?)
```

Parses a JSON string into a Luau value.
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

Serializes a Luau value to a JSON string.

- An array-like table (sequential integer keys from 1) becomes a JSON array.
- Any other table becomes a JSON object, keeping only string keys.
- `indent` controls formatting. The default is compact.
  A positive number indents with that many spaces, and `-1` indents with tabs.

```lua
geode.json.dump({ x = true })   -- '{"x":true}'
geode.json.dump({ 1, 2, 3 }, 2) -- pretty-printed array, 2-space indent
```

## Limits

Nesting depth and parse size are capped.

See [Limits and errors](../cpp/limits-and-errors.md).

## Related

- [Getting started overview](../../getting-started/overview.md)
- [Examples](../../getting-started/examples.md)
- [mod](mod.md)
- [fs](fs.md)
- [Globals](globals.md)
- [Limits and errors](../cpp/limits-and-errors.md)

## Source

- `src/bindings/geode/GeodeSmallBindings.cpp`
- `src/bindings/geode/JsonConvert.hpp`
- `tools/luau_codegen/extra_bindings/json.dluau`
