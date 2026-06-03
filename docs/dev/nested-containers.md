# Nested containers

## Summary

Codegen binds only audited, shallow nested shapes.
There is no generic recursive container marshalling.

Policy constants live in `tools/luau_codegen/model/nested_containers.py`.
Tests live in `tests/luau_codegen/test_nested_containers.py`.

## Luau shapes

**Map with scalar key and vector value** (normal dictionary):

- `gd::unordered_map<int, gd::vector<LabelGameObject*>>` -> `{ [number]: { LabelGameObject? } }`

**Map with pair key and vector value** (entry list, same as [pair containers](pair-containers.md)):

- `gd::map<std::pair<int, int>, gd::vector<GroupCommandObject2*>>` -> `{ { first: number, second: number, value: { GroupCommandObject2? } } }`

**Nested primitive vector** (read-only field getter):

- `gd::vector<gd::vector<int>*>` -> `{ { number } }`

`T*` must be a bound `CCObject` descendant or a registered opaque handle (for example `GroupCommandObject2*`).

## Runtime

| C++ API | Role |
| --- | --- |
| `checkMapValue` / `pushMapValue` | `gd::vector<...>` map values use vector helpers inside one template |
| `pushNestedPrimitiveVectorPointers` | Read-only `gd::vector<gd::vector<int>*>` fields |

Map field setters still clear and re-insert (`assignMap`, `assignUnorderedMap`, pair-key assign helpers).
No whole-container `operator=`.

## Codegen

`convert/type_map.py`:

- `allow_nested_map_value()` gates map values that are `vector_view` with object or opaque elements
- `nested_primitive_vector_view` for `gd::vector<gd::vector<int>*>`
- `allow_nested_primitive_vector_outer()` matches that outer type only

`convert/marshalling.py` emits map helpers and `pushNestedPrimitiveVectorPointers` when needed.

Nested primitive vector fields register as readonly getters (same pattern as `vector_view` fields).

## Rejected

- `gd::map<int, gd::vector<int>>` (primitive vector as map value)
- `gd::vector<gd::vector<gd::vector<...>>>` (deep nesting)
- `gd::vector<gd::vector<bool>*>` (non-audited inner element)
- Nested containers inside set elements or pair components

## Baseline fields

These shapes were skipped before nested support. After regen they should bind:

| Field | Type | Notes |
| --- | --- | --- |
| `m_labelObjects` | `gd::unordered_map<int, gd::vector<LabelGameObject*>>` | Binds |
| `m_timeLabelObjects` | `gd::unordered_map<int, gd::vector<LabelGameObject*>>` | Binds |
| `m_unkMap770` | `gd::map<std::pair<int, int>, gd::vector<GroupCommandObject2*>>` | Binds |
| `m_sectionSizes` | `gd::vector<gd::vector<int>*>` | Read-only getter |
| `m_nonEffectObjectsSizes` | `gd::vector<gd::vector<int>*>` | Read-only getter |
| `m_collisionBlockSectionSizes` | `gd::vector<gd::vector<int>*>` | Read-only getter |

`audit_field_skips.py` should drop these from skip output. Policy skips stay.

## Verification

1. `PYTHONPATH=tools python -m unittest discover -s tests/luau_codegen -p "test_*.py"`
2. Regenerate `types/geode.d.luau` through the build
3. `PYTHONPATH=tools python tools/scripts/audit_field_skips.py`
4. Confirm nested map and size fields bind, deep nested vectors still skipped

## Source

- `tools/luau_codegen/convert/type_map.py`
- `tools/luau_codegen/convert/marshalling.py`
- `tools/luau_codegen/model/nested_containers.py`
- `src/lua/bindings/framework/ContainerTables.hpp`
- `tools/scripts/audit_field_skips.py`
- [Codegen](codegen.md)
- [Pair containers](pair-containers.md)
