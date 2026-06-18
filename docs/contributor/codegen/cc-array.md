# CCArray methods

## Summary

`cocos2d::CCArray*` stays a normal `CCArray` usertype in Luau.
Codegen adds a small set of read methods from `Geode/cocos/cocoa/CCArray.h`.

## Luau shape

`CCArray` exposes Cocos read methods:

- `count()`
- `capacity()`
- `objectAtIndex(index)`
- `indexOfObject(object)`
- `containsObject(object)`
- `firstObject()`
- `lastObject()`
- `randomObject()`

Example:

```lua
local lists = localLevelManager:getCreatedLists(0)
if not lists or lists:count() == 0 then return nil end

local first = lists:objectAtIndex(0)
```

`objectAtIndex` uses Cocos indexes. The first item is index `0`.

## Runtime

These methods bind as normal `CCArray` methods.
Return values such as `getCreatedLists()` still return `CCArray?`, not a table view.

`objectAtIndex`, `firstObject`, `lastObject`, and `randomObject` return `CCObject*` in C++.
When pushing these to Lua, codegen tries to keep the object's right type,
so if the element is a subclass like `GJLevelList` and it is registered, Lua gets that type.
Generated stubs still use `CCObject?`, but you can annotate like `:: GJLevelList?` for type checking.
If an exact runtime type is not registered, LuauAPI uses the closest registered match.
It falls back to `CCNode` for node objects, then `CCObject`.

## Rejected

- `CCArray.data` and `cocos2d::ccArray*` internals
- Writable array helpers until retain and release behavior is audited
- Broad conversion of `CCArray*` returns into Lua arrays

## Verification

1. `PYTHONPATH=tools python -m unittest discover -s tests/luau_codegen -p "test_geode_ccarray.py"`
2. Regenerate `types/geode.d.luau` and confirm `CCArray` has `count` and `objectAtIndex`.
3. Confirm `CCArray.data` remains skipped.

## Related

- [Codegen](codegen.md)
- [ccCArray read-only fields](cc-c-array.md)

## Source

- `tools/luau_codegen/parse/geode_sdk.py`
- `tools/luau_codegen/parse/collect.py`
- `types/geode.d.luau`
