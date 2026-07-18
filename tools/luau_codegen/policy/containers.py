from __future__ import annotations

from luau_codegen.convert.type_map import COMPOSITE_KINDS, TypeInfo
from luau_codegen.model.nested_containers import AUDITED_POINTER_GRID_KIND


_CONTAINER_KINDS = COMPOSITE_KINDS | {AUDITED_POINTER_GRID_KIND, "cc_c_array_view"}
OUT_CONTAINER_KINDS = _CONTAINER_KINDS - {"pair", "tuple"}


def _readonly_container_arg(info: TypeInfo) -> bool:
    return info.kind == "cc_c_array_view"


def _value_element_vector(info: TypeInfo) -> bool:
    return (
        info.kind == "vector"
        and info.element_type is not None
        and info.element_type.kind == "value"
    )


def is_out_container(info: TypeInfo) -> bool:
    return info.is_out and info.kind in OUT_CONTAINER_KINDS


def container_supported_as_arg(info: TypeInfo, ret_kind: str) -> bool:
    if _readonly_container_arg(info):
        return False
    if info.kind not in _CONTAINER_KINDS:
        return True
    if info.kind in ("pair", "tuple"):
        return True
    if info.is_out or info.is_vector_ptr:
        return ret_kind == "void" or _value_element_vector(info)
    return True


def container_supported_as_return(info: TypeInfo) -> bool:
    if info.kind not in _CONTAINER_KINDS:
        return True
    if info.kind in ("pair", "tuple"):
        return True
    if info.is_vector_ptr and _value_element_vector(info):
        return True
    if (
        info.is_out
        and not info.is_vector_ptr
        and info.kind in ("map", "unordered_map", "set", "unordered_set")
    ):
        return True
    return not info.is_out and not info.is_vector_ptr
