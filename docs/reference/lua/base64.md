# base64

## Summary

`geode.utils.base64` encodes and decodes base64 text.
Every function takes an optional `variant` that picks the alphabet and padding.
Use the `Variant` table for the values.

| Name | Value | Notes |
| --- | --- | --- |
| `Normal` | 0 | standard alphabet |
| `NormalNoPad` | 1 | standard alphabet, no padding |
| `Url` | 2 | url safe alphabet |
| `UrlWithPad` | 3 | url safe alphabet, with padding |

A bad variant value raises a Lua error.
The decode functions return `nil` and an error message on bad input, so you can handle them without `pcall`.

## encode

```lua
geode.utils.base64.encode(data: string, variant: number?) -> string
```

Encodes a string into base64. The default variant is `UrlWithPad`.

## decode

```lua
geode.utils.base64.decode(data: string, variant: number?) -> (string?, string?)
```

Decodes base64 into raw bytes, returned as a Luau string.
Whitespace is skipped. Decoding stops at the first NUL byte or `=` padding marker.
Padding is ignored for `Normal`, which behaves like `NormalNoPad`.
Returns the bytes, or `nil` and an error message. The default variant is `Url`.

## decodeString

```lua
geode.utils.base64.decodeString(data: string, variant: number?) -> (string?, string?)
```

Same bytes as `decode`, returned as a Luau string.
Does not validate that the result is valid UTF-8 text.

## Variant

```lua
geode.utils.base64.Variant -> { Normal: number, NormalNoPad: number, Url: number, UrlWithPad: number }
```

A table of the variant values.

## Example

```lua
local b64 = geode.utils.base64

local text = b64.encode("hello", b64.Variant.UrlWithPad)
local back, err = b64.decodeString(text, b64.Variant.UrlWithPad)
if not back then return print(err) end
print(back) -- hello
```

## Related

- [geode.utils](utils.md)
- [clipboard](clipboard.md)
- [permission](permission.md)
- [Globals](globals.md)

## Source

- `src/bindings/geode/GeodeSmallBindings.cpp`
