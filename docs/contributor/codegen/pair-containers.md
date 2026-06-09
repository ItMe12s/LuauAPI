# Pair containers

## Summary

Codegen and `ContainerTables.hpp` bind `std::pair` inside Geode containers.
Pair bodies are record tables. Maps with a pair key use an entry list, not dictionary keys.
Policy constants live in `tools/luau_codegen/model/pair_design.py`.
Tests live in `tests/luau_codegen/test_pair_design.py`.

## Luau shapes

- Pair body (standalone, vector element, set element, or map value):
  - `std::pair<int, int>` becomes `{ first: number, second: number }`,
  - `std::pair<int, cocos2d::CCObject*>` becomes `{ first: number, second: CCObject? }`.
- Map with scalar key and pair value (normal dictionary):
  - `gd::unordered_map<int, std::pair<int, int>>` becomes `{ [number]: { first: number, second: number } }`.
- Map with pair key (entry list, because Lua cannot use tables as keys):

```lua
-- gd::map<std::pair<int, int>, float>
{
    { first = 1, second = 2, value = 0.5 },
    { first = 3, second = 4, value = 1.0 },
}
```

  Stub type pattern: `{ { first: K1, second: K2, value: V } }`.

- Vector or set of pairs (array of pair records):
  - `gd::vector<std::pair<int, int>>` becomes `{ { first: number, second: number } }`.

Do not use positional `{ T1, T2 }`, string keys, or `{ [pairTable]: V }`.

## Runtime

| C++ API | Role |
| --- | --- |
| `checkPair` / `pushPair` | One `std::pair` record |
| `checkMap` / `pushMap` | Scalar keys, pair values use `checkPair` inside `checkMapValue` |
| `checkPairKeyMap` / `pushPairKeyMap` | `gd::map<std::pair<K1,K2>, V>` entry list |
| `checkUnorderedPairKeyMap` / `pushUnorderedPairKeyMap` | Unordered variant |
| `assignPairKeyMap` / `assignUnorderedPairKeyMap` | Field setters for pair-key maps |

Read requires `first` and `second` on pair records. Object pointer members allow nil. Push uses borrowed usertypes.
Field setters still clear and re-insert. No whole-container `operator=`.

## Codegen

In `convert/type_map.py`:

- Pairs are detected with `TypeInfo.kind == "pair"` from `std::pair<First, Second>`.
- Pairs are allowed in:
  - vector elements
  - set elements
  - map values
- Pair map keys use entry-list `lua_type`.

In `convert/marshalling.py`:

- Emits `checkPair`, `pushPair`, and helpers for pair-key maps as needed.

`first` and `second` members follow map value rules:

- Primitives
- Wideint as string
- Strings
- Enums as number
- Gated value structs
- Bound object pointers as `T?`

Still unsupported inside pairs:

- `void*`
- Unbound raw pointers
- Nested containers
- Denied value structs

## Known gaps

These baseline pair fields were skipped before pair support. After regen they bind:

| Field | Type | Notes |
| --- | --- | --- |
| `m_targetGroups` | `gd::unordered_map<int, std::pair<int, int>>` | Binds |
| `m_unkMap578` | `gd::unordered_map<int, std::pair<double, double>>` | Binds |
| `m_destroyObjectValues` | `gd::map<std::pair<int, int>, std::pair<float, float>>` | Binds |
| `m_accountIDForIcon` | `gd::map<std::pair<int, UnlockType>, int>` | Binds |
| `m_unkMap770` | `gd::map<std::pair<int, int>, gd::vector<GroupCommandObject2*>>` | Binds via nested containers |

## SEL pairs

`(CCObject* target, SEL_* selector)` collapsing is unrelated and stays in `convert/sel_args.py`.

## Verification

1. `PYTHONPATH=tools python -m unittest discover -s tests/luau_codegen -p "test_*.py"`
2. Regenerate `types/geode.d.luau` through the build.
3. `PYTHONPATH=tools python -m luau_codegen --bindings <bindings-dir> --audit-report-out audit.md --platform win`
4. Confirm pair fields bind, pair-key stubs use the entry list, policy skips unchanged.

## Related

- [Nested containers](nested-containers.md)
- [Codegen](codegen.md)

## Source

- `tools/luau_codegen/convert/type_map.py`
- `tools/luau_codegen/convert/marshalling.py`
- `tools/luau_codegen/model/pair_design.py`
- `src/lua/bindings/framework/ContainerTables.hpp`
- `tools/luau_codegen/emit/audit.py`
