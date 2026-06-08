from __future__ import annotations

from typing import TYPE_CHECKING, Dict, Optional

from luau_codegen.convert.type_normalization import normalize_type, template_inner
from luau_codegen.model.cc_c_array import (
    CC_C_ARRAY_POINTER_TYPES,
    proven_cc_c_array_element,
)
from luau_codegen.model.nested_containers import (
    allow_nested_map_value,
    allow_nested_primitive_vector_outer,
)
from luau_codegen.model.pair_design import (
    PAIR_COMPONENT_KINDS,
    pair_key_map_entry_lua_type,
    pair_lua_type,
)
from luau_codegen.parse.broma import Class, split_top_level

if TYPE_CHECKING:
    from luau_codegen.convert.type_map import TypeInfo
    from luau_codegen.model.codegen_context import CodegenContext

_PRIMITIVE_VECTOR_ELEMENT_KINDS = frozenset(
    {"bool", "number", "wideint", "string", "enum", "value", "pair"}
)

_STD_ARRAY_ELEMENT_KINDS = _PRIMITIVE_VECTOR_ELEMENT_KINDS

STD_ARRAY_MAX_SIZE = 400

_MAP_KEY_KINDS = frozenset({"bool", "number", "wideint", "string", "enum"})

_MAP_VALUE_KINDS = _PRIMITIVE_VECTOR_ELEMENT_KINDS | {"object"}

_SET_ELEMENT_KINDS = _PRIMITIVE_VECTOR_ELEMENT_KINDS | {"object"}

_NESTED_CONTAINER_KINDS = frozenset(
    {
        "vector_view",
        "primitive_vector",
        "map",
        "unordered_map",
        "set",
        "unordered_set",
    }
)

_MAP_CONTAINER_PREFIXES = (
    ("gd::map", "map"),
    ("gd::unordered_map", "unordered_map"),
)

_SET_CONTAINER_PREFIXES = (
    ("gd::set", "set"),
    ("gd::unordered_set", "unordered_set"),
)


def _classify_arg(
    t: str,
    object_classes: Dict[str, Class],
    *,
    owner_class: str = "",
    field_name: str = "",
    ctx: CodegenContext | None = None,
):
    from luau_codegen.convert.type_classification import classify_arg

    return classify_arg(
        t,
        object_classes,
        owner_class=owner_class,
        field_name=field_name,
        ctx=ctx,
    )


def _is_nested_container(info: TypeInfo) -> bool:
    return info.kind in _NESTED_CONTAINER_KINDS


def _map_value_nested_allowed(key: TypeInfo, value: TypeInfo) -> bool:
    if not _is_nested_container(value):
        return False
    return allow_nested_map_value(key, value)


def _pair_component_lua_type(info: TypeInfo) -> str:
    if info.kind == "object":
        return f"{info.class_name}?"
    if info.kind == "pair":
        if info.key_type is None or info.value_type is None:
            return info.lua_type
        return pair_lua_type(
            _pair_component_lua_type(info.key_type),
            _pair_component_lua_type(info.value_type),
        )
    return info.lua_type


def _map_value_lua_type(value: TypeInfo) -> str:
    if value.kind in (
        "primitive_vector",
        "vector_view",
        "nested_primitive_vector_view",
    ):
        return value.lua_type
    if value.kind == "object":
        return f"{value.class_name}?"
    if value.kind == "pair":
        if value.key_type is None or value.value_type is None:
            return value.lua_type
        return pair_lua_type(
            _pair_component_lua_type(value.key_type),
            _pair_component_lua_type(value.value_type),
        )
    return value.lua_type


def _map_lua_type(key: TypeInfo, value: TypeInfo) -> str:
    if key.kind == "pair":
        if key.key_type is None or key.value_type is None:
            return key.lua_type
        entry = pair_key_map_entry_lua_type(
            _pair_component_lua_type(key.key_type),
            _pair_component_lua_type(key.value_type),
            _map_value_lua_type(value),
        )
        return f"{{ {entry} }}"
    return f"{{ [{key.lua_type}]: {_map_value_lua_type(value)} }}"


def _set_element_lua_type(element: TypeInfo) -> str:
    if element.kind == "object":
        return f"{element.class_name}?"
    if element.kind == "pair":
        if element.key_type is None or element.value_type is None:
            return element.lua_type
        return pair_lua_type(
            _pair_component_lua_type(element.key_type),
            _pair_component_lua_type(element.value_type),
        )
    return element.lua_type


def parse_std_pair(
    n: str,
    object_classes: Dict[str, Class],
    type_info_cls,
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    inner = template_inner(n, "std::pair")
    if inner is None:
        return None
    parts = split_top_level(inner)
    if len(parts) != 2:
        return None
    first = _classify_arg(parts[0], object_classes, ctx=ctx)
    second = _classify_arg(parts[1], object_classes, ctx=ctx)
    if first is None or second is None:
        return None
    if first.kind not in PAIR_COMPONENT_KINDS or second.kind not in PAIR_COMPONENT_KINDS:
        return None
    if _is_nested_container(first) or _is_nested_container(second):
        return None
    return type_info_cls(
        "pair",
        f"std::pair<{first.cxx_type}, {second.cxx_type}>",
        pair_lua_type(
            _pair_component_lua_type(first),
            _pair_component_lua_type(second),
        ),
        key_type=first,
        value_type=second,
    )


def parse_map_container(
    n: str,
    prefix: str,
    kind: str,
    object_classes: Dict[str, Class],
    type_info_cls,
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    inner = template_inner(n, prefix)
    if inner is None:
        return None
    parts = split_top_level(inner)
    if len(parts) != 2:
        return None
    key = _classify_arg(parts[0], object_classes, ctx=ctx)
    value = _classify_arg(parts[1], object_classes, ctx=ctx)
    if key is None or value is None:
        return None
    if key.kind == "pair":
        if key.key_type is None or key.value_type is None:
            return None
        if _is_nested_container(value):
            if not _map_value_nested_allowed(key, value):
                return None
        elif value.kind not in _MAP_VALUE_KINDS:
            return None
    else:
        if key.kind not in _MAP_KEY_KINDS or _is_nested_container(key):
            return None
        if _is_nested_container(value):
            if not _map_value_nested_allowed(key, value):
                return None
        elif value.kind not in _MAP_VALUE_KINDS:
            return None
    return type_info_cls(
        kind,
        f"{prefix}<{key.cxx_type}, {value.cxx_type}>",
        _map_lua_type(key, value),
        key_type=key,
        value_type=value,
    )


def parse_set_container(
    n: str,
    prefix: str,
    kind: str,
    object_classes: Dict[str, Class],
    type_info_cls,
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    inner = template_inner(n, prefix)
    if inner is None:
        return None
    parts = split_top_level(inner)
    if len(parts) != 1:
        return None
    element = _classify_arg(parts[0], object_classes, ctx=ctx)
    if element is None or element.kind not in _SET_ELEMENT_KINDS:
        return None
    if _is_nested_container(element):
        return None
    return type_info_cls(
        kind,
        f"{prefix}<{element.cxx_type}>",
        f"{{ {_set_element_lua_type(element)} }}",
        element_type=element,
    )


def parse_container(
    n: str,
    object_classes: Dict[str, Class],
    type_info_cls,
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    for prefix, kind in _MAP_CONTAINER_PREFIXES:
        parsed = parse_map_container(n, prefix, kind, object_classes, type_info_cls, ctx=ctx)
        if parsed is not None:
            return parsed
    for prefix, kind in _SET_CONTAINER_PREFIXES:
        parsed = parse_set_container(n, prefix, kind, object_classes, type_info_cls, ctx=ctx)
        if parsed is not None:
            return parsed
    return None


def with_container_ref_flags(
    info: TypeInfo,
    type_info_cls,
    *,
    is_ref: bool,
    is_out: bool,
    is_vector_ptr: bool = False,
) -> TypeInfo:
    return type_info_cls(
        info.kind,
        info.cxx_type,
        info.lua_type,
        info.class_name,
        is_ref,
        is_out,
        is_vector_ptr,
        info.callback_ret,
        info.callback_args,
        element_type=info.element_type,
        key_type=info.key_type,
        value_type=info.value_type,
        array_size=info.array_size,
    )


def parse_std_array(
    n: str,
    object_classes: Dict[str, Class],
    type_info_cls,
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    inner = template_inner(n, "std::array")
    if inner is None:
        return None
    parts = split_top_level(inner)
    if len(parts) != 2:
        return None
    size_str = parts[1].strip()
    if not size_str.isdigit():
        return None
    size = int(size_str)
    if size <= 0 or size > STD_ARRAY_MAX_SIZE:
        return None
    element = _classify_arg(parts[0].strip(), object_classes, ctx=ctx)
    if element is None or element.kind not in _STD_ARRAY_ELEMENT_KINDS:
        return None
    if _is_nested_container(element):
        return None
    return type_info_cls(
        "std_array",
        f"std::array<{element.cxx_type}, {size}>",
        f"{{ {element.lua_type} }}",
        element_type=element,
        array_size=size,
    )


def parse_primitive_vector(
    n: str,
    object_classes: Dict[str, Class],
    type_info_cls,
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    inner = template_inner(n, "gd::vector")
    if inner is None:
        return None
    parts = split_top_level(inner)
    if len(parts) != 1:
        return None
    element = _classify_arg(parts[0], object_classes, ctx=ctx)
    if element is None or element.kind not in _PRIMITIVE_VECTOR_ELEMENT_KINDS:
        return None
    return type_info_cls(
        "primitive_vector",
        f"gd::vector<{element.cxx_type}>",
        f"{{ {element.lua_type} }}",
        element_type=element,
    )


def parse_cc_c_array_view(
    n: str,
    owner_class: str,
    field_name: str,
    object_classes: Dict[str, Class],
    type_info_cls,
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    if normalize_type(n) not in CC_C_ARRAY_POINTER_TYPES:
        return None
    element_short = proven_cc_c_array_element(owner_class, field_name)
    if element_short is None:
        return None
    element = _classify_arg(f"{element_short}*", object_classes, ctx=ctx)
    if element is None or element.kind != "object":
        return None
    return type_info_cls(
        "cc_c_array_view",
        "cocos2d::ccCArray*",
        f"{{ {element.class_name}? }}",
        element.class_name,
        element_type=element,
    )


def parse_vector_view(
    n: str,
    object_classes: Dict[str, Class],
    type_info_cls,
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    inner = template_inner(n, "gd::vector")
    if inner is None:
        return None
    parts = split_top_level(inner)
    if len(parts) != 1:
        return None
    element = _classify_arg(parts[0], object_classes, ctx=ctx)
    if element is None or element.kind not in ("object", "opaque_handle"):
        return None
    if element.kind == "object":
        lua_type = f"{{ {element.class_name}? }}"
        class_name = element.class_name
    else:
        lua_type = f"{{ {element.lua_type}? }}"
        class_name = element.lua_type
    return type_info_cls(
        "vector_view",
        f"gd::vector<{element.cxx_type}>",
        lua_type,
        class_name,
        element_type=element,
    )


def parse_nested_primitive_vector_view(
    n: str,
    object_classes: Dict[str, Class],
    type_info_cls,
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    if not allow_nested_primitive_vector_outer(n):
        return None
    inner = template_inner(n, "gd::vector")
    if inner is None:
        return None
    parts = split_top_level(inner)
    if len(parts) != 1:
        return None
    elem = parts[0].strip()
    if not elem.endswith("*"):
        return None
    inner_vec = parse_primitive_vector(elem[:-1].strip(), object_classes, type_info_cls, ctx=ctx)
    if inner_vec is None or inner_vec.element_type is None:
        return None
    if inner_vec.element_type.kind != "number":
        return None
    return type_info_cls(
        "nested_primitive_vector_view",
        n,
        f"{{ {inner_vec.lua_type} }}",
        element_type=inner_vec,
    )
