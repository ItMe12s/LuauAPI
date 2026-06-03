# Pair containers

## Summary

Codegen and `ContainerTables.hpp` bind `std::pair` inside Geode containers.
Pair bodies are record tables. Maps with a pair key use an entry list, not dictionary keys.

Policy constants live in `tools/luau_codegen/model/pair_design.py`.
Tests live in `tests/luau_codegen/test_pair_design.py`.

## Luau shapes

**Pair body** (standalone, vector element, set element, or map value):

- `std::pair<int, int>` → `{ first: number, second: number }`
- `std::pair<int, UnlockType>` → `{ first: number, second: number }`
- `std::pair<int, cocos2d::CCObject*>` → `{ first: number, second: CCObject? }`

**Map with scalar key and pair value** (normal dictionary):

- `gd::unordered_map<int, std::pair<int, int>>` -> `{ [number]: { first: number, second: number } }`

**Map with pair key** (entry list, because Lua cannot use tables as keys):

```luau
-- gd::map<std::pair<int, int>, float>
{
    { first = 1, second = 2, value = 0.5 },
    { first = 3, second = 4, value = 1.0 },
}
```

Stub type pattern: `{ { first: K1, second: K2, value: V } }`.

**Vector or set of pairs** (array of pair records, same as other sets):

- `gd::vector<std::pair<int, int>>` → `{ { first: number, second: number } }`
- `gd::set<std::pair<int, int>>` → `{ { first: number, second: number } }`

Do not use positional `{ T1, T2 }`, string keys, or `{ [pairTable]: V }`.

## Runtime

| C++ API | Role |
| --- | --- |
| `checkPair` / `pushPair` | One `std::pair` record |
| `checkMap` / `pushMap` | Scalar keys. Pair values use `checkPair` inside `checkMapValue` |
| `checkPairKeyMap` / `pushPairKeyMap` | `gd::map<std::pair<K1,K2>, V>` entry list |
| `checkUnorderedPairKeyMap` / `pushUnorderedPairKeyMap` | Unordered variant |
| `assignPairKeyMap` / `assignUnorderedPairKeyMap` | Field setters for pair-key maps |

Read requires `first` and `second` on pair records. Missing fields error.
Object pointer members allow nil (null). Push uses borrowed usertypes.

Field setters still clear and re-insert (`assignMap`, `assignSet`, `assignPrimitiveVector`, pair-key assign helpers). No whole-container `operator=`.

## Codegen

`convert/type_map.py`:

- `TypeInfo.kind == "pair"` from `std::pair<First, Second>`
- `pair` allowed in vector elements, set elements, and map values
- Pair map keys use entry-list `lua_type`, not `{ [pair]: V }`

`convert/marshalling.py` emits `checkPair`, `pushPair`, and pair-key map helpers when needed.

## Components

`first` and `second` follow map **value** rules:

- Primitives, wideint as string, strings, enums as number, gated value structs, bound object pointers as `T?`

Still unsupported inside pairs: `void*`, unbound raw pointers, nested containers, denied value structs.

## Known gaps

These baseline pair-related fields were skipped before pair support. After regen, four of five should bind:

| Field | Type | Notes |
| --- | --- | --- |
| `m_targetGroups` | `gd::unordered_map<int, std::pair<int, int>>` | Binds |
| `m_unkMap578` | `gd::unordered_map<int, std::pair<double, double>>` | Binds |
| `m_destroyObjectValues` | `gd::map<std::pair<int, int>, std::pair<float, float>>` | Binds |
| `m_accountIDForIcon` | `gd::map<std::pair<int, UnlockType>, int>` | Binds |
| `m_unkMap770` | `gd::map<std::pair<int, int>, gd::vector<GroupCommandObject2*>>` | Binds via [nested containers](nested-containers.md) |

`audit_field_skips.py` groups remaining pair skips in the `pair` bucket.

## SEL pairs

`(CCObject* target, SEL_* selector)` collapsing is unrelated.
It stays in `convert/sel_args.py`.

## Verification

1. `PYTHONPATH=tools python -m unittest discover -s tests/luau_codegen -p "test_*.py"`
2. Regenerate `types/geode.d.luau` through the build
3. `PYTHONPATH=tools python tools/scripts/audit_field_skips.py`
4. Confirm pair fields bind, pair-key stubs use the entry list, policy skips unchanged

## Source

- `tools/luau_codegen/convert/type_map.py`
- `tools/luau_codegen/convert/marshalling.py`
- `tools/luau_codegen/model/pair_design.py`
- `src/lua/bindings/framework/ContainerTables.hpp`
- `tools/scripts/audit_field_skips.py`
- [Codegen](codegen.md)
