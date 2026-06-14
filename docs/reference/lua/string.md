# string

## Summary

`geode.utils.string` holds small string helpers from Geode.
They do not replace Luau's built-in string library.
Signatures match the generated stub at `types/geode.d.luau` after build.

## trim

```lua
geode.utils.string.trim(s: string) -> string
geode.utils.string.trim(s: string, chars: string) -> string
```

Removes whitespace from both ends, or removes any character in `chars` from both ends.

## trimLeft

```lua
geode.utils.string.trimLeft(s: string) -> string
geode.utils.string.trimLeft(s: string, chars: string) -> string
```

Removes leading whitespace or characters in `chars`.

## trimRight

```lua
geode.utils.string.trimRight(s: string) -> string
geode.utils.string.trimRight(s: string, chars: string) -> string
```

Removes trailing whitespace or characters in `chars`.

## toLower

```lua
geode.utils.string.toLower(s: string) -> string
```

Returns a lowercase copy of `s`.

## toUpper

```lua
geode.utils.string.toUpper(s: string) -> string
```

Returns an uppercase copy of `s`.

## contains

```lua
geode.utils.string.contains(s: string, sub: string) -> boolean
```

Returns whether `s` contains `sub`.

## startsWith

```lua
geode.utils.string.startsWith(s: string, prefix: string) -> boolean
```

Returns whether `s` starts with `prefix`.

## endsWith

```lua
geode.utils.string.endsWith(s: string, suffix: string) -> boolean
```

Returns whether `s` ends with `suffix`.

## replace

```lua
geode.utils.string.replace(s: string, from: string, to: string) -> string
```

Replaces every occurrence of `from` with `to`.

## remove

```lua
geode.utils.string.remove(s: string, sub: string) -> string
```

Removes every occurrence of `sub`.

## filter

```lua
geode.utils.string.filter(s: string, chars: string) -> string
```

Keeps only characters that appear in `chars`.

## normalize

```lua
geode.utils.string.normalize(s: string) -> string
```

Collapses repeated spaces according to Geode's rules.

## count

```lua
geode.utils.string.count(s: string, c: number) -> string
```

Counts how many times the character `c` appears in `s`.
Pass `c` as a byte value, for example `string.byte("e")`.
The result is a decimal integer string because wide integers return as strings in LuauAPI.

## Example

```lua
local str = geode.utils.string

local id = str.trim("  hello  ")
print(str.startsWith(id, "hel"))            -- true
print(str.replace(id, "hello", "world"))    -- world
print(str.count("hello", string.byte("l"))) -- "2"
```

## Related

- [Getting started overview](../../getting-started/overview.md)
- [geode.utils](utils.md)
- [globals](globals.md)

## Source

- `tools/luau_codegen/model/free_fn_sources.py`
- Generated free-function bindings at build time
