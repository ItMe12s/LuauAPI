# Reference: base64

## Summary

This page lists `geode.utils.base64`. It encodes and decodes base64 text.

Every function takes an optional `variant`. The variant picks the alphabet and padding.
Use the `Variant` table for the values.

| Name | Value | Notes |
| --- | --- | --- |
| `Normal` | 0 | standard alphabet |
| `NormalNoPad` | 1 | standard alphabet, no padding |
| `Url` | 2 | url safe alphabet |
| `UrlWithPad` | 3 | url safe alphabet, with padding |

A bad variant value raises a Lua error. The decode functions return `nil` and an error
message on bad input, so you can handle them without `pcall`.

## encode

```lua
geode.utils.base64.encode(data: string, variant: number?) -> string
```

Encodes a string into base64. The default variant is `UrlWithPad`.

## decode

```lua
geode.utils.base64.decode(data: string, variant: number?) -> (string?, string?)
```

Decodes base64 into the raw bytes, returned as a string. Returns the bytes, or `nil` and
an error message. The default variant is `Url`.

## decodeString

```lua
geode.utils.base64.decodeString(data: string, variant: number?) -> (string?, string?)
```

Same as `decode`, but does not check that the result is valid text. Returns the string, or
`nil` and an error message.

## Variant

```lua
geode.utils.base64.Variant -> { Normal: number, NormalNoPad: number, Url: number, UrlWithPad: number }
```

A table of the variant values.

## Example

```lua
local b64 = geode.utils.base64

local text = b64.encode("hello", b64.Variant.UrlWithPad)
print(text)

local back, err = b64.decodeString(text, b64.Variant.UrlWithPad)
if not back then
    print(err)
    return
end
print(back) -- hello
```

## Related

- [Reference: permission](permission.md)
- [Reference: json](json.md)
- [Lua overview](../overview.md)

## Source

- `src/lua/bindings/geode/GeodeBase64Binding.cpp`
