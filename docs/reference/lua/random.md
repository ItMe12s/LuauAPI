# random

## Summary

`geode.utils.random` generates random strings and UUIDs from Geode's PRNG.
These are not cryptographically secure OS random bytes unless Geode says otherwise elsewhere.

Length arguments use the wide-integer binding rule. The stub types them as `string`.
At runtime you must pass a decimal integer string such as `"16"`, not a bare number.

## generateUUID

```lua
geode.utils.random.generateUUID() -> string
```

Returns a random UUID v4 string such as `"550e8400-e29b-41d4-a716-446655440000"`.

## generateHexString

```lua
geode.utils.random.generateHexString(length: string) -> string
```

Returns a random lowercase hex string with `length` characters.
Pass `"16"` for a 16-character token.

## generateAlphanumericString

```lua
geode.utils.random.generateAlphanumericString(length: string) -> string
```

Returns a random string using `[0-9A-Za-z]` with `length` characters.

## generateString

```lua
geode.utils.random.generateString(length: string, alphabet: string) -> string
```

Returns a random string with `length` characters, picking only from `alphabet`.
`alphabet` must not be empty.

## Example

```lua
local rand = geode.utils.random

local id = rand.generateUUID()
print(id)

local token = rand.generateHexString("16")
print(token)
```

## Related

- [Getting started overview](../../getting-started/overview.md)
- [geode.utils](utils.md)
- [globals](globals.md)

## Source

- `tools/luau_codegen/model/free_fn_sources.py`
- Generated free-function bindings at build time
