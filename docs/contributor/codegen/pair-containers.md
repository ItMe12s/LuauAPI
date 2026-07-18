# Pair containers

## Summary

Codegen and `ContainerTables.hpp` bind `std::pair` as a recursive by-value composite.
Pair bodies are record tables. Maps with a pair key use an entry list, not dictionary keys.

## Luau shapes

- Pair body (standalone, vector element, set element, or map value):
  - `std::pair<int, int>` becomes `{ first: number, second: number }`.
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
  Entry order from an unordered map is unspecified.

- Vector or set of pairs (array of pair records):
  - `gd::vector<std::pair<int, int>>` becomes `{ { first: number, second: number } }`.

Do not use positional `{ T1, T2 }`, serialized string keys, or `{ [pairTable]: V }`.

## Runtime

| C++ API | Role |
| --- | --- |
| `checkContainerValue<T>` / `pushContainerValue<T>` | Recursive pair records and their descendants |
| `assignContainerValue` | ABI-safe by-value field replacement |

Pair fields are checked by their declared types.
Nullable object and opaque members may be `nil` or missing.
Bound object pointers push borrowed usertypes.
Opaque handles remain borrowed.
Field setters update recursively through `assignContainerValue`.
No whole-container `operator=` is used.

## Codegen

In `convert/type_classification.py` and `convert/type_map.py`:

- Pairs are detected with `TypeInfo.kind == "pair"` from `std::pair<First, Second>`.
- Pair members accept any value allowed by the recursive container grammar.
- Pair map keys use entry-list `lua_type`.
- A map key pair remains restricted to the non-container pair-key leaf set: scalar, enum, value struct,
  or bound object pointer members.

`convert/marshalling.py` emits the generic recursive container entry points.

`first` and `second` members follow recursive value rules:

- Primitives
- Wideint as string
- `std::string` and `gd::string`
- Enums as number
- Gated value structs
- Bound object pointers as `T?`
- Opaque handles as `T?`
- Supported containers, pairs, and tuples

Still unsupported inside pairs:

- `void*`
- Unbound raw pointers
- Denied value structs
- Callback, delegate, task, result, and `ccCArray` nodes
- Descendant container pointers

## Field coverage

Generated pair field examples:

| Field | Type | Notes |
| --- | --- | --- |
| `m_targetGroups` | `gd::unordered_map<int, std::pair<int, int>>` | Binds |
| `m_unkMap578` | `gd::unordered_map<int, std::pair<double, double>>` | Binds |
| `m_destroyObjectValues` | `gd::map<std::pair<int, int>, std::pair<float, float>>` | Binds |
| `m_accountIDForIcon` | `gd::map<std::pair<int, UnlockType>, int>` | Binds |
| `m_unkMap770` | `gd::map<std::pair<int, int>, gd::vector<GroupCommandObject2*>>` | Binds via recursive containers |

## SEL pairs

`(CCObject* target, SEL_* selector)` collapsing is unrelated and stays in `convert/sel_args.py`.

## Related

- [Recursive containers](nested-containers.md)
- [Codegen](codegen.md)
- [CCArray methods](cc-array.md)
- [Testing](../testing.md)

## Source

- `tools/luau_codegen/convert/type_classification.py`
- `tools/luau_codegen/convert/type_map.py`
- `tools/luau_codegen/convert/marshalling.py`
- `tools/luau_codegen/model/pair_design.py`
- `src/framework/stack/ContainerTables.hpp`
