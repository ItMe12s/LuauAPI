from __future__ import annotations

from luau_codegen.convert.type_map import TypeInfo


_CONTAINER_KINDS = frozenset(
    {
        "vector_view",
        "nested_primitive_vector_view",
        "nested_bool_vector_view",
        "nested_object_vector_view",
        "nested_object_grid_view",
        "map_vector",
        "cc_c_array_view",
        "primitive_vector",
        "std_array",
        "map",
        "unordered_map",
        "set",
        "unordered_set",
    }
)


def _readonly_container_arg(info: TypeInfo) -> bool:
    return info.kind in ("nested_primitive_vector_view", "cc_c_array_view")


def _value_element_primitive_vector(info: TypeInfo) -> bool:
    return (
        info.kind == "primitive_vector"
        and info.element_type is not None
        and info.element_type.kind == "value"
    )


def container_supported_as_arg(info: TypeInfo, ret_kind: str) -> bool:
    if _readonly_container_arg(info):
        return False
    if info.kind not in _CONTAINER_KINDS:
        return True
    if info.is_out or info.is_vector_ptr:
        return ret_kind == "void" or _value_element_primitive_vector(info)
    return True


def container_supported_as_return(info: TypeInfo) -> bool:
    if info.kind not in _CONTAINER_KINDS:
        return True
    if info.is_vector_ptr and _value_element_primitive_vector(info):
        return True
    if (
        info.is_out
        and not info.is_vector_ptr
        and info.kind in ("map", "unordered_map", "set", "unordered_set")
    ):
        return True
    return not info.is_out and not info.is_vector_ptr
