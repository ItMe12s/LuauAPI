# geode.utils

## Summary

`geode.utils` groups small helper libraries under the `geode.utils` prefix.
Large modules have their own reference page. Smaller ones are documented below.

Signatures match [types/geode.d.luau](../../../types/geode.d.luau).
Some Geode C++ integer sizes appear as `string` in the stub and use integer-string arguments at runtime.

## Libraries

| Module | Page | Role |
| --- | --- | --- |
| `geode.utils.web` | [web](web.md) | Async HTTP requests |
| `geode.utils.base64` | [base64](base64.md) | Base64 encode and decode |
| `geode.utils.permission` | [permission](permission.md) | OS permission checks |
| `geode.utils.clipboard` | [clipboard](clipboard.md) | System clipboard text |
| `geode.utils.string` | [string](string.md) | String helpers |
| `geode.utils.random` | [random](random.md) | Random strings and UUIDs |
| `geode.utils.game` | [game](game.md) | Exit, restart, uninstaller |

Related top-level helpers outside this prefix:

- [json](json.md) as `geode.json`
- [VersionInfo](version-info.md) as `geode.VersionInfo`

## Top-level helpers

These functions sit directly on `geode.utils`, not in a child table.

```lua
geode.utils.formatSystemError(code: number) -> string
geode.utils.getDisplayFactor() -> number
geode.utils.getInputTimestamp() -> number
geode.utils.getSafeAreaRect() -> CCRect
```

`formatSystemError` turns a system error code into text.
`getDisplayFactor` returns the ratio of physical pixels to logical pixels on one axis.
`getInputTimestamp` returns the latest input timestamp in seconds.
`getSafeAreaRect` returns the safe area rectangle relative to the window size.

## platform

```lua
geode.utils.platform.getString() -> string
geode.utils.platform.isWine() -> boolean
```

`getString` returns a human-readable platform label used in crash logs and compatibility checks.
`isWine` returns whether the Windows build is running under Wine. It is always false on other platforms.

## thread

```lua
geode.utils.thread.getDefaultName() -> string
geode.utils.thread.getName() -> string
geode.utils.thread.setName(name: string) -> ()
```

Read or set the current thread name for debugging and logging.
`getName` returns an empty string when no name was assigned.

## Related

- [Getting started overview](../../getting-started/overview.md)
- [globals](globals.md)
- [web](web.md)
- [base64](base64.md)
- [permission](permission.md)
- [clipboard](clipboard.md)
- [string](string.md)
- [random](random.md)
- [game](game.md)

## Source

- `src/bindings/geode/GeodeSmallBindings.cpp`
- `src/bindings/geode/web/GeodeWebBinding.cpp`
- `tools/luau_codegen/extra_bindings/web.dluau`
- `tools/luau_codegen/model/free_fn_sources.py`
- `tools/luau_codegen/emit/luau_types/manual_fields.py`
- `build/luauapi-gen/bindings_free_functions.cpp`
- `types/geode.d.luau`
