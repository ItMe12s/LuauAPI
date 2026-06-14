# geode.utils

## Summary

`geode.utils` groups small helper libraries under the `geode.utils` prefix.
Each child library has its own reference page.

## Libraries

| Module | Page | Role |
| --- | --- | --- |
| `geode.utils.web` | [web](web.md) | Async HTTP requests |
| `geode.utils.base64` | [base64](base64.md) | Base64 encode and decode |
| `geode.utils.permission` | [permission](permission.md) | OS permission checks |

Related top-level helpers outside this prefix:

- [json](json.md) as `geode.json`
- [VersionInfo](version-info.md) as `geode.VersionInfo`

## Related

- [Globals](globals.md)
- [web](web.md)
- [base64](base64.md)
- [permission](permission.md)

## Source

- `src/bindings/geode/GeodeSmallBindings.cpp`
- `src/bindings/geode/web/GeodeWebBinding.cpp`
- `tools/luau_codegen/extra_bindings/web.dluau`
- `tools/luau_codegen/emit/luau_types/manual_fields.py`
