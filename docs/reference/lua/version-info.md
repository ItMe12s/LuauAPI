# VersionInfo

## Summary

`geode.VersionInfo` parses and compares version strings. It uses a small part of semver.
A version looks like `v1.2.3` or `v1.2.3-beta.1`. The functions take strings, so you do not build a version by hand.
They return `nil` and an error message on a bad version, so you can handle them without `pcall`.

## parse

```lua
geode.VersionInfo.parse(version: string) -> ({ major: number, minor: number, patch: number }?, string?)
```

Parses a version string into its parts. Returns a table, or `nil` and an error message.

## compare

```lua
geode.VersionInfo.compare(a: string, b: string) -> (number?, string?)
```

Compares two versions. Returns `-1` if `a` is older, `0` if equal, `1` if `a` is newer.
Returns `nil` and an error message if either string is bad.

## matches

```lua
geode.VersionInfo.matches(constraint: string, version: string) -> (boolean?, string?)
```

Tests a version against a constraint such as `>=v1.2.0`. Returns `true` or `false`, or `nil` and an
error message.

## Example

```lua
local v = geode.VersionInfo

local parts = v.parse("v1.2.3")
print(parts.major, parts.minor, parts.patch) -- 1 2 3

print(v.compare("v1.0.0", "v1.2.0")) -- -1
print(v.matches(">=v1.2.0", "v1.3.0")) -- true
```

## Related

- [mod](mod.md)
- [Sharing APIs between mods](sharing-apis.md)

## Source

- `src/lua/bindings/geode/GeodeVersionBinding.cpp`
