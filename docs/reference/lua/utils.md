# geode.utils

## Summary

`geode.utils` groups small helper libraries under the `geode.utils` prefix.
Large modules have their own reference page. Smaller ones are documented on this page.

For signatures, use editor autocomplete from [type stubs](type-stubs.md).
Some Geode C++ integer sizes appear as `string` in the stub and use integer-string arguments at runtime.

## Libraries

| Module | Page | Role |
| --- | --- | --- |
| `geode.utils` | Top-level helpers | Error text, display factor, env vars, input time, safe area |
| `geode.utils.platform` | platform | Platform label and Wine detection |
| `geode.utils.thread` | thread | Thread name for logging |
| `geode.utils.clipboard` | clipboard | Clipboard read and write |
| `geode.utils.random` | random | UUIDs and random strings |
| `geode.utils.string` | string | Trim, case, search, replace helpers |
| `geode.utils.web` | [web](web.md) | Async HTTP requests |
| `geode.utils.base64` | [base64](base64.md) | Base64 encode and decode |
| `geode.utils.permission` | [permission](permission.md) | OS permission checks |
| `geode.utils.game` | [game](game.md) | Exit, restart, uninstaller |

Related top-level helpers outside this prefix:

- [json](json.md) as `geode.json`
- [VersionInfo](version-info.md) as `geode.VersionInfo`

## Top-level helpers

These functions sit directly on `geode.utils`, not in a child table.

| Function | Role |
| --- | --- |
| `formatSystemError` | Turn a system error code into text |
| `getDisplayFactor` | Ratio of physical pixels to logical pixels on one axis |
| `getEnvironmentVariable` | Read a process environment variable by name. Not available on iOS |
| `getInputTimestamp` | Latest input timestamp in seconds |
| `getSafeAreaRect` | Safe area rectangle relative to the window size |

## platform

| Function | Role |
| --- | --- |
| `getString` | Human-readable platform label for crash logs and compatibility checks |
| `isWine` | Whether the Windows build runs under Wine. Always false on other platforms |

## thread

Read or set the current thread name for debugging and logging.

| Function | Role |
| --- | --- |
| `getDefaultName` | Default thread name |
| `getName` | Current name, or the default when no custom name was set |
| `setName` | Assign a custom name |

## clipboard

`geode.utils.clipboard` reads and writes the system clipboard as plain text.
Use it for paste into a text field or copy a result for the user.

| Function | Role |
| --- | --- |
| `read` | Current clipboard text. Empty string when empty or read fails |
| `write` | Copy text to the clipboard. Returns success as a boolean |

```lua
local clip = geode.utils.clipboard

local paste = clip.read()
if paste ~= "" then
    print("clipboard:", paste)
end

clip.write("copied from Luau")
```

## random

`geode.utils.random` generates random strings and UUIDs from Geode's PRNG.
These are not cryptographically secure OS random bytes unless Geode says otherwise elsewhere.

Length arguments use the wide-integer binding rule. The stub types them as `string`.
At runtime you must pass a decimal integer string such as `"16"`, not a bare number.

| Function | Role |
| --- | --- |
| `generateUUID` | Random UUID v4 string |
| `generateHexString` | Random lowercase hex string with the given length |
| `generateAlphanumericString` | Random string using `[0-9A-Za-z]` |
| `generateString` | Random string from a custom alphabet. `alphabet` must not be empty |

```lua
local rand = geode.utils.random

local id = rand.generateUUID()
print(id)

local token = rand.generateHexString("16")
print(token)
```

## string

`geode.utils.string` holds small string helpers from Geode.
They do not replace Luau's built-in string library.

| Function | Role |
| --- | --- |
| `trim`, `trimLeft`, `trimRight` | Remove whitespace or a custom character set from ends |
| `toLower`, `toUpper` | Case-changed copy |
| `contains`, `startsWith`, `endsWith` | Substring tests |
| `replace` | Swap every occurrence of one substring for another |
| `remove` | Delete every occurrence of a substring |
| `filter` | Keep only characters in a given set |
| `normalize` | Collapse repeated spaces per Geode rules |
| `count` | Count byte occurrences. Pass `c` as a byte value such as `string.byte("e")`. Result is a decimal integer string |

```lua
local str = geode.utils.string

local id = str.trim("  hello  ")
print(str.startsWith(id, "hel"))            -- true
print(str.replace(id, "hello", "world"))    -- world
print(str.count("hello", string.byte("l"))) -- "2"
```

## Related

- [Getting started](../../getting-started/overview.md)
- [globals](globals.md)
- [web](web.md)
- [base64](base64.md)
- [permission](permission.md)
- [game](game.md)

## Source

- `src/bindings/geode/GeodeSmallBindings.cpp`
- `src/bindings/geode/web/GeodeWebCore.cpp`
- `tools/luau_codegen/extra_bindings/web.dluau`
- `tools/luau_codegen/model/free_fn_sources.py`
- `tools/luau_codegen/emit/luau_types/manual_fields.py`
- Generated free-function bindings and `types/geode.d.luau` at build time
