# clipboard

## Summary

`geode.utils.clipboard` reads and writes the system clipboard as plain text.
Use it for paste into a text field or copy a result for the user.

## read

```lua
geode.utils.clipboard.read() -> string
```

Returns the current clipboard text.
Returns an empty string when the clipboard is empty or the read fails.

## write

```lua
geode.utils.clipboard.write(text: string) -> boolean
```

Copies `text` to the clipboard. Returns `true` on success and `false` on failure.

## Example

```lua
local clip = geode.utils.clipboard

local paste = clip.read()
if paste ~= "" then
    print("clipboard:", paste)
end

clip.write("copied from Luau")
```

## Related

- [Getting started](../../getting-started/overview.md)
- [geode.utils](utils.md)
- [globals](globals.md)

## Source

- `tools/luau_codegen/model/free_fn_sources.py`
- Generated free-function bindings at build time
