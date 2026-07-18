# Recursive containers

## Summary

Codegen supports recursive by-value container trees.
For example, `gd::map<int, gd::vector<int>>` becomes `{ [number]: { number } }`.
Descendant composite pointers are unsupported except in seven audited fields.

## Grammar

```text
Composite := gd::vector<Value>
           | gd::map<Key, Value> | gd::unordered_map<Key, Value>
           | gd::set<Value> | gd::unordered_set<Value>
           | std::array<Value, N>
           | std::pair<Value, Value>
           | std::tuple<Value...>

Value     := supported scalar, enum, value struct, bound object pointer, or opaque handle
           | Composite

Key       := supported scalar
           | std::pair<non-container pair-key leaf, non-container pair-key leaf>
```

The non-container pair-key leaf set is scalar, enum, supported value struct, or bound object pointer.
There is no explicit nesting limit.
Supported `std::array` sizes are listed in [Limits and errors](../../reference/cpp/limits-and-errors.md).

Only the listed `gd` containers and `std::array`, `std::pair`, and `std::tuple` take part.
`std::vector`, `std::map`, and `std::set` remain unsupported.

Direct `gd::vector<Object*>` and `gd::vector<Opaque*>` values use read-only sequence views.
Other recursive values use table snapshots.
Plain Lua table rules apply to snapshots.
A `nil` indexed leaf creates a hole.
A `nil` value in a dictionary-shaped map removes that entry.

## Luau shapes

- `gd::map<int, gd::vector<int>>` becomes `{ [number]: { number } }`.
- `gd::vector<gd::unordered_map<int, bool>>` becomes `{ { [number]: boolean } }`.
- `std::array<gd::set<gd::string>, 4>` becomes `{ { string } }`.
- `std::pair<A, B>` stays `{ first: A, second: B }`.
- A map with a pair key becomes `{ { first: K1, second: K2, value: V } }`.
- `std::tuple<>` becomes `{}`.
- A homogeneous tuple becomes `{ T }`.
- A heterogeneous tuple becomes `{ (T1 | T2 | ...) }`.

Luau array types cannot describe each tuple index separately.
The runtime still requires exact tuple arity and checks every position.
It rejects holes and extra numeric positions.
`std::array` tables also require exactly the declared size with no holes or extra numeric positions.
Array output order from unordered sets and unordered pair-key maps is unspecified.

## Surfaces

By-value trees bind on class fields, method arguments, method returns, and free functions.
Existing root pointer and out-parameter behavior stays unchanged.
Hooks remain container-free.

Recursive values reject:

- any descendant `Composite*`
- composite map keys other than the restricted pair form
- `Container**`
- nested `ccCArray`
- callback, delegate, task, result, and unsupported leaves
- `char*`, string-view, and `ZStringView` leaves

Seven audited `GJBaseGameLayer` pointer-grid fields use one field-only adapter.
The read-only fields are `m_sectionSizes`, `m_nonEffectObjectsSizes`, and `m_collisionBlockSectionSizes`.
The writable fields are `m_nonEffectObjectsFlags`, `m_collisionBlockSections`, `m_sections`, and `m_nonEffectObjects`.
Null children through this adapter still appear as empty tables.
Lua cannot distinguish those null children from empty children.
Writing `{}` creates or keeps a non-null empty row.
Codegen emits the adapter only for these seven fields.

## Runtime and codegen

`TypeInfo` stores the full tree through `element_type`, `key_type`, `value_type`, and `tuple_types`.
One tree walk drives dependency, inaccessible-class, hook, and value-struct checks.

Generated recursive values use:

- `checkContainerValue<T>` for table checks
- `pushContainerValue<T>` for table pushes
- `assignContainerValue` for recursive field updates

`assignContainerValue` avoids whole-container assignment for Geode container ABI support.
It clears and reinserts vectors, maps, and sets.
Arrays, pairs, and tuples update recursively in place.
The seven pointer-grid fields classify as one `audited_pointer_grid` kind.
It emits `pushAuditedPointerGrid`, `checkAuditedPointerGrid<Actual>`, and `assignAuditedPointerGrid`.
These helpers preserve the existing read, write, and null-child behavior without broadening recursive container support.

## Related

- [Pair containers](pair-containers.md)
- [Codegen](codegen.md)
- [ccCArray read-only fields](cc-c-array.md)
- [Testing](../testing.md)
- [Game objects](../../reference/lua/game-objects.md)
- [Limits and errors](../../reference/cpp/limits-and-errors.md)

## Source

- `tools/luau_codegen/convert/type_classification.py`
- `tools/luau_codegen/convert/type_map.py`
- `tools/luau_codegen/convert/marshalling.py`
- `tools/luau_codegen/model/nested_containers.py`
- `tools/luau_codegen/emit/bindings/class_file.py`
- `src/framework/stack/ContainerTables.hpp`
