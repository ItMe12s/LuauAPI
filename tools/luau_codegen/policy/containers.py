from __future__ import annotations

from luau_codegen.convert.type_map import TypeInfo


_CONTAINER_KINDS = frozenset(
    {
        "vector_view",
        "nested_primitive_vector_view",
        "primitive_vector",
        "std_array",
        "map",
        "unordered_map",
        "set",
        "unordered_set",
    }
)


def _nested_vector_view_readonly(info: TypeInfo) -> bool:
    return info.kind == "nested_primitive_vector_view"


def container_supported_as_arg(info: TypeInfo, ret_kind: str) -> bool:
    if _nested_vector_view_readonly(info):
        return False
    if info.kind not in _CONTAINER_KINDS:
        return True
    if info.is_out or info.is_vector_ptr:
        return ret_kind == "void"
    return True


def container_supported_as_return(info: TypeInfo) -> bool:
    if info.kind not in _CONTAINER_KINDS:
        return True
    return not info.is_out and not info.is_vector_ptr


def vector_view_supported_as_return(info: TypeInfo) -> bool:
    return container_supported_as_return(info)


def vector_view_supported_as_arg(info: TypeInfo, ret_kind: str) -> bool:
    return container_supported_as_arg(info, ret_kind)
