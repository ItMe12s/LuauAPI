from __future__ import annotations

from luau_codegen.convert.type_map import TypeInfo


_CONTAINER_KINDS = frozenset({"vector_view", "primitive_vector"})


def vector_view_supported_as_return(info: TypeInfo) -> bool:
    if info.kind not in _CONTAINER_KINDS:
        return True
    return not info.is_out and not info.is_vector_ptr


def vector_view_supported_as_arg(info: TypeInfo, ret_kind: str) -> bool:
    if info.kind not in _CONTAINER_KINDS:
        return True
    if info.is_out or info.is_vector_ptr:
        return ret_kind == "void"
    return True
