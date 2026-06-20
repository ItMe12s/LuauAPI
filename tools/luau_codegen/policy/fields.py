from __future__ import annotations

import re
from typing import TYPE_CHECKING, Dict

if TYPE_CHECKING:
    from luau_codegen.model.codegen_context import CodegenContext

from luau_codegen.parse.broma import Class, Field
from luau_codegen.model.denylist import INACCESSIBLE_CLASSES
from luau_codegen.convert.type_map import (
    TypeInfo,
    classify_arg,
    classify_return,
    normalize_type,
)
from luau_codegen.policy.containers import (
    _CONTAINER_KINDS,
    container_supported_as_arg,
    container_supported_as_return,
)


INACCESSIBLE_FIELDS = {
    ("CCObject", "m_nChildIndex"),
}

# Getter-only container fields skip setter arg gates.
_READONLY_FIELD_CONTAINER_KINDS = frozenset(
    {
        "nested_primitive_vector_view",
        "cc_c_array_view",
    }
)

# Mutable pointer container fields bind getters/setters.
_MUTABLE_POINTER_FIELD_CONTAINER_KINDS = frozenset(
    {
        "primitive_vector",
        "std_array",
        "map",
        "unordered_map",
        "set",
        "unordered_set",
    }
)


def field_key(cls: Class, field: Field) -> str:
    return f"{cls.qualified_name}.{field.name}:{field.type}"


def field_applies_on_platform(field: Field, platform: str) -> bool:
    if field.platforms is None:
        return True
    return platform in field.platforms


def _is_function_pointer(t: str) -> bool:
    return bool(re.search(r"\(\s*\*", normalize_type(t)))


def _mutable_pointer_field_container(info: TypeInfo) -> bool:
    return info.kind in _MUTABLE_POINTER_FIELD_CONTAINER_KINDS and (
        info.is_out or info.is_vector_ptr
    )


def bindable_field(
    field: Field,
    objects: Dict[str, Class],
    cls: Class | None = None,
    ctx: CodegenContext | None = None,
) -> tuple[bool, str, TypeInfo | None, TypeInfo | None]:
    normalized = normalize_type(field.type)
    if field.access != "public":
        return False, "inaccessible", None, None
    if cls and (cls.name, field.name) in INACCESSIBLE_FIELDS:
        return False, "inaccessible-field", None, None
    if field.count > 1:
        return False, "array", None, None
    if "&" in normalized:
        return False, "reference", None, None
    if _is_function_pointer(normalized):
        return False, "function-pointer", None, None
    owner_class = cls.name if cls else ""
    arg = classify_arg(
        field.type,
        objects,
        owner_class=owner_class,
        field_name=field.name,
        ctx=ctx,
    )
    ret = classify_return(
        field.type,
        objects,
        owner_class=owner_class,
        field_name=field.name,
        ctx=ctx,
    )
    if arg is None:
        return False, f"unsupported-arg:{field.type}", None, ret
    if ret is None:
        return False, f"unsupported-return:{field.type}", arg, None
    if ret.kind in _CONTAINER_KINDS and not container_supported_as_return(ret):
        if not _mutable_pointer_field_container(ret):
            return False, f"unsupported-return:{field.type}", arg, ret
    readonly_container_field = ret.kind in _READONLY_FIELD_CONTAINER_KINDS
    if (
        arg.kind in _CONTAINER_KINDS
        and not readonly_container_field
        and not container_supported_as_arg(arg, ret.kind)
        and not _mutable_pointer_field_container(arg)
    ):
        return False, f"unsupported-arg:{field.type}", arg, ret
    if arg.kind == "string" and arg.cxx_type.endswith("*"):
        return False, "string-pointer", arg, ret
    if ret.kind == "object" and ret.class_name in INACCESSIBLE_CLASSES:
        return False, f"inaccessible-type:{ret.class_name}", arg, ret
    if arg.kind == "object" and arg.class_name in INACCESSIBLE_CLASSES:
        return False, f"inaccessible-type:{arg.class_name}", arg, ret
    return True, "", arg, ret


def field_skipped_object_ref(
    field: Field,
    objects: Dict[str, Class],
    skipped_classes: set[str],
    cls: Class | None = None,
    ctx: CodegenContext | None = None,
) -> str:
    blocked = skipped_classes | INACCESSIBLE_CLASSES
    owner_class = cls.name if cls else ""
    for info in (
        classify_arg(
            field.type,
            objects,
            owner_class=owner_class,
            field_name=field.name,
            ctx=ctx,
        ),
        classify_return(
            field.type,
            objects,
            owner_class=owner_class,
            field_name=field.name,
            ctx=ctx,
        ),
    ):
        if info and info.kind == "object" and info.class_name in blocked:
            return info.class_name
    return ""
