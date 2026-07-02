# Nested containers

## Summary

Codegen binds only audited, shallow nested shapes. There is no generic recursive container marshalling.
Read-only nested primitive size vectors and read-write GJ grids, map vectors, and tuple sets are in scope.
Policy constants live in `tools/luau_codegen/model/nested_containers.py`.
Classification gates live in `tools/luau_codegen/convert/type_classification.py`.
Tests live in `tests/luau_codegen/test_nested_containers.py` and `tests/luau_codegen/test_gj_grid_fields.py`.

## Luau shapes

- Map with scalar key and vector value (normal dictionary):
  - `gd::unordered_map<int, gd::vector<LabelGameObject*>>` becomes `{ [number]: { LabelGameObject? } }`
- Map with pair key and vector value (entry list, same as [Pair containers](pair-containers.md)):
  - `gd::map<std::pair<int, int>, gd::vector<GroupCommandObject2*>>` becomes
    `{ { first: number, second: number, value: { GroupCommandObject2? } } }`
- Nested primitive vector (read-only field getter):
  - `gd::vector<gd::vector<int>*>` becomes `{ { number } }`
- Nested bool vector (read-write field):
  - `gd::vector<gd::vector<bool>*>` becomes `{ { boolean } }`
- Nested object vector (read-write field, borrowed inner elements):
  - `gd::vector<gd::vector<GameObject*>*>` becomes `{ { GameObject? } }`
- Nested object grid (read-write field, borrowed inner elements):
  - `gd::vector<gd::vector<gd::vector<GameObject*>*>*>` becomes `{ { { GameObject? } } }`
- Map vector (read-write field):
  - `gd::vector<gd::unordered_map<int,int>>` becomes `{ { [number]: number } }`
- Tuple set element (read-write field):
  - `gd::set<std::tuple<int, int, int>>` becomes `{ { number, number, number } }`

`T*` must be a bound `CCObject` descendant or a registered opaque handle.

## Runtime

| C++ API | Role |
| --- | --- |
| `checkMapValue` / `pushMapValue` | `gd::vector<...>` map values use vector helpers inside one template |
| `pushNestedPrimitiveVectorPointers` | Read-only `gd::vector<gd::vector<int>*>` fields |
| `checkNestedPrimitiveVectorPointers` / `assignNestedPrimitiveVectorPointers` | Read-write nested bool grids |
| `pushNestedObjectVectorPointers` / `checkNestedObjectVectorPointers` / `assignNestedObjectVectorPointers` | 2-level object grids |
| `pushNestedObjectGridPointers` / `checkNestedObjectGridPointers` / `assignNestedObjectGridPointers` | 3-level object grids |
| `checkTupleTableInt3` / `pushTupleTableInt3` | `std::tuple<int,int,int>` set elements |

Map field setters still clear and re-insert (`assignMap`, `assignUnorderedMap`, pair-key assign helpers).
No whole-container `operator=`.

## Codegen

In `model/nested_containers.py` and `convert/type_classification.py`:

- `allow_nested_map_value()` gates map values that are `vector_view` with object or opaque elements.
- `nested_primitive_vector_view` handles read-only `gd::vector<gd::vector<int>*>`.
- `nested_bool_vector_view`, `nested_object_vector_view`, and `nested_object_grid_view` handle audited GJ grid fields with getters and setters.
- `map_vector` handles read-write `gd::vector<gd::unordered_map<int,int>>`.
- `parse_std_tuple()` handles audited `std::tuple<int,int,int>` set elements for read-write set fields.
- `allow_nested_*()` helpers match exact outer types only.

`convert/marshalling.py` emits map helpers and nested push/check helpers.
`emit/bindings/class_file.py` emits setter assign paths for read-write nested fields.
Nested primitive vector fields register as read-only getters.

## Rejected

- `gd::map<int, gd::vector<int>>` (primitive vector as map value)
- Generic deep nesting beyond audited GJ grid shapes
- Nested containers inside pair components
- `std::array` fields above 2000 elements

## Baseline fields

`BASELINE_NESTED_*` in `model/nested_containers.py` are coverage examples only.
Codegen does not read them. Shape helpers gate binding.

These shapes were skipped before nested support. After regen they should bind:

| Field | Type | Notes |
| --- | --- | --- |
| `m_labelObjects` | `gd::unordered_map<int, gd::vector<LabelGameObject*>>` | Binds |
| `m_timeLabelObjects` | `gd::unordered_map<int, gd::vector<LabelGameObject*>>` | Binds |
| `m_unkMap770` | `gd::map<std::pair<int, int>, gd::vector<GroupCommandObject2*>>` | Binds |
| `m_sectionSizes` | `gd::vector<gd::vector<int>*>` | Read-only getter |
| `m_nonEffectObjectsSizes` | `gd::vector<gd::vector<int>*>` | Read-only getter |
| `m_collisionBlockSectionSizes` | `gd::vector<gd::vector<int>*>` | Read-only getter |
| `m_nonEffectObjectsFlags` | `gd::vector<gd::vector<bool>*>` | Read-write |
| `m_collisionBlockSections` | `gd::vector<gd::vector<GameObject*>*>` | Read-write |
| `m_sections` / `m_nonEffectObjects` | `gd::vector<gd::vector<gd::vector<GameObject*>*>*>` | Read-write |
| `m_spawnRemapTriggers` | `gd::vector<gd::unordered_map<int,int>>` | Read-write |
| `m_spawnTuples` | `gd::set<std::tuple<int, int, int>>` | Read-write |
| `m_varianceValues` | `std::array<float, 2000>` | Read-write (`STD_ARRAY_MAX_SIZE` 2000) |

## Verification

1. `PYTHONPATH=tools python -m unittest discover -s tests/luau_codegen -p "test_*.py"`
2. Regenerate `types/geode.d.luau` through the build.
3. `PYTHONPATH=tools python -m luau_codegen --bindings <bindings-dir> --audit-report-out audit.md --platform win`
4. Confirm nested map, size, GJ grid, and exotic fields bind. Generic deep nesting should still skip.

## Related

- [Pair containers](pair-containers.md)
- [ccCArray read-only fields](cc-c-array.md)
- [Codegen](codegen.md)

## Source

- `tools/luau_codegen/convert/type_map.py`
- `tools/luau_codegen/convert/type_classification.py`
- `tools/luau_codegen/convert/marshalling.py`
- `tools/luau_codegen/model/nested_containers.py`
- `tools/luau_codegen/emit/bindings/class_file.py`
- `src/framework/stack/ContainerTables.hpp`
- `tools/luau_codegen/emit/audit.py`
