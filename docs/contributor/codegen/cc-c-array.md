# ccCArray read-only fields

## Summary

`cocos2d::ccCArray*` stores raw `void*` slots with no retain or release.
Codegen binds it only for audited dispatcher handler queues where the element type and parent ownership are known.
Policy lives in `tools/luau_codegen/model/cc_c_array.py`.
Runtime lives in `src/framework/view/ReadOnlyCCArrayView.hpp`.

## Luau shape

Proven fields expose a read-only sequence:

- `cocos2d::ccCArray*` on `CCKeyboardDispatcher.m_pHandlersToAdd` becomes `{ CCKeyboardHandler? }`

The same pattern applies to `CCKeypadHandler`, `CCMouseHandler`, and `CCTouchHandler`
on the matching dispatcher `m_pHandlersToAdd` and `m_pHandlersToRemove` fields in the allowlist.
`CCTouchDispatcher` only exposes `m_pHandlersToRemove`, not `m_pHandlersToAdd`.

## Runtime

| C++ API | Role |
| --- | --- |
| `pushReadOnlyCCArrayView<T>` | Field getter, borrows elements while dispatcher `self` is alive |

The view userdata indexes `1..num` from `ccCArray::arr`, pushes borrowed usertypes, and errors on `__newindex`.

## Rejected

- `cocos2d::ccArray*` internals (for example `CCArray.data`)
- Unknown `ccCArray*` fields without an allowlist entry
- Writable setters until retain semantics are designed

## Related

- [Codegen](codegen.md)
- [Recursive containers](nested-containers.md)
- [Pair containers](pair-containers.md)
- [Testing](../testing.md)

## Source

- `tools/luau_codegen/model/cc_c_array.py`
- `tools/luau_codegen/convert/type_map.py`
- `src/framework/view/ReadOnlyCCArrayView.hpp`
