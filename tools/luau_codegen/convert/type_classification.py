from __future__ import annotations

import dataclasses
from typing import Dict, Optional

from luau_codegen.convert.type_map import (
    CALLBACK_ALIASES,
    CLASS_CALLBACK_ALIASES,
    COMPOSITE_KINDS,
    NUMERIC_TYPES,
    OPAQUE_HANDLE_TYPES,
    SEL_TYPES,
    STRING_TYPES,
    VALUE_TYPES,
    WIDE_INTEGER_TYPES,
    TypeInfo,
    callback_inner,
    cxx_class_name,
    enum_cxx_type,
    is_out_reference,
    is_reference_type,
    iter_type_tree,
    normalize_type,
    resolve_object_class,
    sel_lua_type,
    sel_variant,
    strip_ref,
    template_inner,
)
from luau_codegen.model import delegate_specs as _delegate_specs
from luau_codegen.model.cc_c_array import (
    CC_C_ARRAY_POINTER_TYPES,
    proven_cc_c_array_element,
)
from luau_codegen.model.codegen_context import CodegenContext
from luau_codegen.model.domain import short_name
from luau_codegen.model.nested_containers import (
    AUDITED_POINTER_GRID_KIND,
    audited_gj_pointer_field_spec,
)
from luau_codegen.model.pair_design import (
    PAIR_COMPONENT_KINDS,
    pair_key_map_entry_lua_type,
    pair_lua_type,
)
from luau_codegen.parse.broma import Class, split_arg, split_top_level

STD_ARRAY_MAX_SIZE = 2000

_MAP_KEY_KINDS = frozenset({"bool", "number", "wideint", "string", "enum"})

_RECURSIVE_LEAF_KINDS = frozenset(
    {"bool", "number", "wideint", "string", "enum", "value", "object", "opaque_handle"}
)

_RECURSIVE_STRING_CXX_TYPES = frozenset({"std::string", "gd::string"})

_MAP_CONTAINER_PREFIXES = (
    ("gd::map", "map"),
    ("gd::unordered_map", "unordered_map"),
)

_SET_CONTAINER_PREFIXES = (
    ("gd::set", "set"),
    ("gd::unordered_set", "unordered_set"),
)


def _is_recursive_value(info: TypeInfo) -> bool:
    if info.kind == "string":
        return info.cxx_type in _RECURSIVE_STRING_CXX_TYPES
    return info.kind in _RECURSIVE_LEAF_KINDS or info.kind in COMPOSITE_KINDS


def _nested_lua_type(info: TypeInfo) -> str:
    if info.kind == "object":
        return f"{info.class_name}?"
    if info.kind == "opaque_handle":
        return info.lua_type if info.lua_type.endswith("?") else f"{info.lua_type}?"
    return info.lua_type


def _classify_child(
    raw: str,
    object_classes: Dict[str, Class],
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    normalized = normalize_type(raw)
    if normalized.endswith("*"):
        pointee = _parse_composite(normalized[:-1].strip(), object_classes, ctx=ctx)
        if pointee is not None:
            return None
    return classify_arg(raw, object_classes, ctx=ctx)


def _is_map_key(info: TypeInfo) -> bool:
    if info.kind in _MAP_KEY_KINDS:
        return info.kind != "string" or info.cxx_type in _RECURSIVE_STRING_CXX_TYPES
    return (
        info.kind == "pair"
        and info.key_type is not None
        and info.value_type is not None
        and info.key_type.kind in PAIR_COMPONENT_KINDS
        and info.value_type.kind in PAIR_COMPONENT_KINDS
        and _is_recursive_value(info.key_type)
        and _is_recursive_value(info.value_type)
    )


def _map_lua_type(key: TypeInfo, value: TypeInfo) -> str:
    if key.kind == "pair":
        if key.key_type is None or key.value_type is None:
            return key.lua_type
        entry = pair_key_map_entry_lua_type(
            _nested_lua_type(key.key_type),
            _nested_lua_type(key.value_type),
            _nested_lua_type(value),
        )
        return f"{{ {entry} }}"
    return f"{{ [{key.lua_type}]: {_nested_lua_type(value)} }}"


def _tuple_lua_type(items: tuple[TypeInfo, ...]) -> str:
    if not items:
        return "{}"
    variants = tuple(dict.fromkeys(_nested_lua_type(item) for item in items))
    if len(variants) == 1:
        return f"{{ {variants[0]} }}"
    return f"{{ ({' | '.join(variants)}) }}"


def parse_std_pair(
    n: str,
    object_classes: Dict[str, Class],
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    inner = template_inner(n, "std::pair")
    if inner is None:
        return None
    parts = split_top_level(inner)
    if len(parts) != 2:
        return None
    first = _classify_child(parts[0], object_classes, ctx=ctx)
    second = _classify_child(parts[1], object_classes, ctx=ctx)
    if first is None or second is None:
        return None
    if not _is_recursive_value(first) or not _is_recursive_value(second):
        return None
    return TypeInfo(
        "pair",
        f"std::pair<{first.cxx_type}, {second.cxx_type}>",
        pair_lua_type(
            _nested_lua_type(first),
            _nested_lua_type(second),
        ),
        key_type=first,
        value_type=second,
    )


def parse_map_container(
    n: str,
    prefix: str,
    kind: str,
    object_classes: Dict[str, Class],
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    inner = template_inner(n, prefix)
    if inner is None:
        return None
    parts = split_top_level(inner)
    if len(parts) != 2:
        return None
    key = _classify_child(parts[0], object_classes, ctx=ctx)
    value = _classify_child(parts[1], object_classes, ctx=ctx)
    if key is None or value is None:
        return None
    if not _is_map_key(key) or not _is_recursive_value(value):
        return None
    return TypeInfo(
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
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    inner = template_inner(n, prefix)
    if inner is None:
        return None
    parts = split_top_level(inner)
    if len(parts) != 1:
        return None
    element = _classify_child(parts[0], object_classes, ctx=ctx)
    if element is None or not _is_recursive_value(element):
        return None
    return TypeInfo(
        kind,
        f"{prefix}<{element.cxx_type}>",
        f"{{ {_nested_lua_type(element)} }}",
        element_type=element,
    )


def parse_container(
    n: str,
    object_classes: Dict[str, Class],
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    for prefix, kind in _MAP_CONTAINER_PREFIXES:
        parsed = parse_map_container(n, prefix, kind, object_classes, ctx=ctx)
        if parsed is not None:
            return parsed
    for prefix, kind in _SET_CONTAINER_PREFIXES:
        parsed = parse_set_container(n, prefix, kind, object_classes, ctx=ctx)
        if parsed is not None:
            return parsed
    return None


def _with_ref_flags(
    info: TypeInfo,
    *,
    is_ref: bool,
    is_out: bool,
) -> TypeInfo:
    return dataclasses.replace(info, is_ref=is_ref, is_out=is_out)


def parse_std_array(
    n: str,
    object_classes: Dict[str, Class],
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
    element = _classify_child(parts[0].strip(), object_classes, ctx=ctx)
    if element is None or not _is_recursive_value(element):
        return None
    return TypeInfo(
        "std_array",
        f"std::array<{element.cxx_type}, {size}>",
        f"{{ {_nested_lua_type(element)} }}",
        element_type=element,
        array_size=size,
    )


def parse_gd_vector(
    n: str,
    object_classes: Dict[str, Class],
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    inner = template_inner(n, "gd::vector")
    if inner is None:
        return None
    parts = split_top_level(inner)
    if len(parts) != 1:
        return None
    element = _classify_child(parts[0], object_classes, ctx=ctx)
    if element is None or not _is_recursive_value(element):
        return None
    kind = "vector_view" if element.kind in ("object", "opaque_handle") else "vector"
    class_name = ""
    if element.kind == "object":
        class_name = element.class_name
    elif element.kind == "opaque_handle":
        class_name = element.lua_type
    return TypeInfo(
        kind,
        f"gd::vector<{element.cxx_type}>",
        f"{{ {_nested_lua_type(element)} }}",
        class_name,
        element_type=element,
    )


def parse_cc_c_array_view(
    n: str,
    owner_class: str,
    field_name: str,
    object_classes: Dict[str, Class],
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    if normalize_type(n) not in CC_C_ARRAY_POINTER_TYPES:
        return None
    element_short = proven_cc_c_array_element(owner_class, field_name)
    if element_short is None:
        return None
    element = classify_arg(f"{element_short}*", object_classes, ctx=ctx)
    if element is None or element.kind != "object":
        return None
    return TypeInfo(
        "cc_c_array_view",
        "cocos2d::ccCArray*",
        f"{{ {element.class_name}? }}",
        element.class_name,
        element_type=element,
    )


def parse_audited_gj_pointer_field(
    n: str,
    owner_class: str,
    field_name: str,
    object_classes: Dict[str, Class],
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    spec = audited_gj_pointer_field_spec(owner_class, field_name, n)
    if spec is None:
        return None
    mirror_cxx, _ = spec
    mirror = _parse_composite(mirror_cxx, object_classes, ctx=ctx)
    if mirror is None:
        return None
    return TypeInfo(
        AUDITED_POINTER_GRID_KIND,
        n,
        mirror.lua_type,
        element_type=mirror,
    )


def parse_std_tuple(
    n: str,
    object_classes: Dict[str, Class],
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    inner = template_inner(n, "std::tuple")
    if inner is None:
        return None
    items: list[TypeInfo] = []
    for raw in split_top_level(inner):
        item = _classify_child(raw, object_classes, ctx=ctx)
        if item is None or not _is_recursive_value(item):
            return None
        items.append(item)
    tuple_types = tuple(items)
    return TypeInfo(
        "tuple",
        f"std::tuple<{', '.join(item.cxx_type for item in tuple_types)}>",
        _tuple_lua_type(tuple_types),
        tuple_types=tuple_types,
    )


def _parse_composite(
    n: str,
    object_classes: Dict[str, Class],
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    for parser in (
        parse_gd_vector,
        parse_std_array,
        parse_container,
        parse_std_pair,
        parse_std_tuple,
    ):
        parsed = parser(n, object_classes, ctx=ctx)
        if parsed is not None:
            return parsed
    return None


def _parse_root_pointer_composite(
    n: str,
    object_classes: Dict[str, Class],
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    for parser in (parse_gd_vector, parse_std_array, parse_container):
        parsed = parser(n, object_classes, ctx=ctx)
        if parsed is not None:
            return parsed
    return None


def _resolve_ctx(ctx: CodegenContext | None) -> CodegenContext:
    if ctx is not None:
        return ctx
    return CodegenContext.static()


def _strip_task_handle_ref(t: str) -> tuple[str, str]:
    raw = normalize_type(t)
    if raw.endswith("&&"):
        return raw[:-2].strip(), "rvalue"
    if raw.endswith("&"):
        inner = raw[:-1].strip()
        if inner.startswith("const ") or inner.endswith(" const"):
            return strip_ref(raw), "const_lvalue"
        return inner, "lvalue"
    return raw, "value"


def _task_handle_inner(n: str) -> Optional[str]:
    for prefix in ("arc::TaskHandle", "TaskHandle"):
        inner = template_inner(n, prefix)
        if inner is not None:
            inner = inner.strip()
            return inner if inner else "void"
    return None


def _task_handle_lua_inner(inner: TypeInfo) -> str:
    if inner.kind == "void":
        return "nil"
    return inner.lua_type


def _parse_task_handle_type(
    t: str,
    object_classes: Dict[str, Class],
    *,
    for_return: bool,
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    raw, ref_kind = _strip_task_handle_ref(t)
    if not for_return:
        return None

    optional_inner = template_inner(raw, "std::optional")
    if optional_inner is not None:
        if ref_kind != "value":
            return None
        task_inner = _task_handle_inner(optional_inner.strip())
        if task_inner is None:
            return None
        inner = classify_return(task_inner, object_classes, ctx=ctx)
        if inner is None:
            return None
        cxx_inner = inner.cxx_type
        return TypeInfo(
            "optional_task_handle",
            f"std::optional<arc::TaskHandle<{cxx_inner}>>",
            f"GeodeTaskHandle<{_task_handle_lua_inner(inner)}>?",
            element_type=inner,
        )

    task_inner = _task_handle_inner(raw)
    if task_inner is None:
        return None
    if for_return and ref_kind != "value":
        return None
    inner = classify_return(task_inner, object_classes, ctx=ctx)
    if inner is None:
        return None
    cxx_inner = inner.cxx_type
    return TypeInfo(
        "task_handle",
        f"arc::TaskHandle<{cxx_inner}>",
        f"GeodeTaskHandle<{_task_handle_lua_inner(inner)}>",
        element_type=inner,
    )


def _parse_result_type(n: str) -> Optional[TypeInfo]:
    s = strip_ref(n)
    for prefix in ("geode::Result<", "Result<"):
        if s.startswith(prefix) and s.endswith(">"):
            inner = s[len(prefix) : -1].strip()
            if inner not in ("", "void"):
                return None
            return TypeInfo("result", "geode::Result<>", "boolean | string")
    return None


def _parse_callback(
    n: str, object_classes: Dict[str, Class], ctx: CodegenContext | None = None
) -> Optional[TypeInfo]:
    inner = callback_inner(n)
    if inner is None:
        return None
    depth = 0
    paren = -1
    for i, c in enumerate(inner):
        if c == "<":
            depth += 1
        elif c == ">":
            depth -= 1
        elif c == "(" and depth == 0:
            paren = i
            break
    if paren == -1:
        return None
    ret_str = inner[:paren].strip()
    pdepth = 0
    close = -1
    for i in range(paren, len(inner)):
        if inner[i] == "(":
            pdepth += 1
        elif inner[i] == ")":
            pdepth -= 1
            if pdepth == 0:
                close = i
                break
    if close == -1:
        return None
    ret_info = classify_return(ret_str, object_classes, ctx=ctx)
    if ret_info is None or any(node.kind in COMPOSITE_KINDS for node in iter_type_tree(ret_info)):
        return None
    arg_infos = []
    for raw in split_top_level(inner[paren + 1 : close]):
        raw = raw.strip()
        if not raw:
            continue
        parsed = split_arg(raw)
        info = classify_arg(parsed.type, object_classes, ctx=ctx)
        if (
            info is None
            or info.kind == "callback"
            or any(node.kind in COMPOSITE_KINDS for node in iter_type_tree(info))
        ):
            return None
        arg_infos.append(info)
    lua_params = ", ".join(f"arg{i}: {ai.lua_type}" for i, ai in enumerate(arg_infos, start=1))
    if ret_info.kind == "void":
        lua_ret = "()"
    else:
        lua_ret = ret_info.lua_type
    return TypeInfo(
        "callback",
        n,
        f"({lua_params}) -> {lua_ret}",
        callback_ret=ret_info,
        callback_args=tuple(arg_infos),
    )


def _delegate_lua_type(spec) -> str:
    fields = []
    for m in spec.methods:
        params = ", ".join(f"arg{i}: {t}" for i, t in enumerate(m.args_lua, start=1))
        ret = m.ret_lua
        fn = f"({params}) -> ()" if ret == "()" else f"({params}) -> {ret}"
        fields.append(f"{m.name}: ({fn})?")
    return "{ " + ", ".join(fields) + " }"


def _with_out_ptr_flags(info: TypeInfo) -> TypeInfo:
    return dataclasses.replace(
        info,
        is_ref=False,
        is_out=True,
        is_vector_ptr=True,
    )


def _classify_core(
    t: str,
    object_classes: Dict[str, Class],
    *,
    for_return: bool,
    ctx: CodegenContext | None = None,
    owner_class: str = "",
    field_name: str = "",
) -> Optional[TypeInfo]:
    resolved = _resolve_ctx(ctx)
    is_ref = is_reference_type(t)
    is_out = is_out_reference(t)
    s = strip_ref(t)
    n = normalize_type(s)
    if n.endswith("*"):
        base = n[:-1].strip()
    else:
        base = short_name(n)

    task_handle = _parse_task_handle_type(t, object_classes, for_return=for_return, ctx=ctx)
    if task_handle is not None:
        return task_handle

    if n.endswith("*"):
        pointer_composite = _parse_root_pointer_composite(base, object_classes, ctx=ctx)
        if pointer_composite is not None:
            return _with_out_ptr_flags(pointer_composite)

    audited_pointer_field = parse_audited_gj_pointer_field(
        n, owner_class, field_name, object_classes, ctx=ctx
    )
    if audited_pointer_field is not None:
        return _with_ref_flags(audited_pointer_field, is_ref=is_ref, is_out=is_out)

    cc_c_array_view = parse_cc_c_array_view(n, owner_class, field_name, object_classes, ctx=ctx)
    if cc_c_array_view is not None:
        return _with_ref_flags(cc_c_array_view, is_ref=is_ref, is_out=is_out)

    composite = _parse_composite(n, object_classes, ctx=ctx)
    if composite is not None:
        return _with_ref_flags(composite, is_ref=is_ref, is_out=is_out)
    if base.startswith("SeedValue"):
        return TypeInfo("seed_value", n, "number", is_ref=is_ref, is_out=is_out)
    if n == "bool":
        return TypeInfo("bool", n, "boolean", is_ref=is_ref, is_out=is_out)
    if n in WIDE_INTEGER_TYPES:
        return TypeInfo("wideint", n, "string", is_ref=is_ref, is_out=is_out)
    if n in NUMERIC_TYPES:
        return TypeInfo("number", n, "number", is_ref=is_ref, is_out=is_out)
    if n in STRING_TYPES:
        return TypeInfo("string", n, "string", is_ref=is_ref, is_out=is_out)
    if n in VALUE_TYPES:
        return TypeInfo(
            "value",
            n,
            VALUE_TYPES.get(n, VALUE_TYPES.get(base, n)),
            is_ref=is_ref,
            is_out=is_out,
        )
    if base in resolved.enum_types or n in resolved.enum_types:
        cxx = enum_cxx_type(n, base, ctx=resolved)
        return TypeInfo("enum", cxx, "number", is_ref=is_ref, is_out=is_out)
    if n in OPAQUE_HANDLE_TYPES:
        return TypeInfo(
            "opaque_handle",
            n,
            OPAQUE_HANDLE_TYPES[n],
            is_ref=is_ref,
            is_out=is_out,
        )
    result = _parse_result_type(n)
    if result is not None:
        return TypeInfo(
            result.kind,
            result.cxx_type,
            result.lua_type,
            is_ref=is_ref,
            is_out=is_out,
        )
    if not for_return:
        callback = _parse_callback(n, object_classes, ctx=ctx)
        if callback is not None:
            return callback
        if normalize_type(t) in SEL_TYPES:
            return TypeInfo(
                "sel",
                n,
                sel_lua_type(n),
                class_name=sel_variant(n),
            )
    if n.endswith("*"):
        spec = _delegate_specs.lookup_delegate(n)
        if spec is not None:
            lua_table = _delegate_lua_type(spec)
            return TypeInfo(
                "delegate",
                n,
                lua_table,
                class_name=spec.lua_name,
                is_ref=is_ref,
                is_out=is_out,
            )
    if n.endswith("*"):
        cls = resolve_object_class(n, object_classes)
        if cls:
            lua_suffix = "?" if for_return else ""
            return TypeInfo(
                "object",
                cxx_class_name(cls) + "*",
                f"{cls.name}{lua_suffix}",
                cls.name,
                is_ref=is_ref,
                is_out=is_out,
            )
    return None


def classify_arg(
    t: str,
    object_classes: Dict[str, Class],
    *,
    owner_class: str = "",
    field_name: str = "",
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    n = normalize_type(t)
    if owner_class:
        class_aliases = CLASS_CALLBACK_ALIASES.get(owner_class, {})
        alias = class_aliases.get(n) or class_aliases.get(short_name(n))
        if alias:
            return classify_arg(
                alias,
                object_classes,
                owner_class=owner_class,
                field_name=field_name,
                ctx=ctx,
            )
    alias = CALLBACK_ALIASES.get(n) or CALLBACK_ALIASES.get(short_name(n))
    if alias:
        return classify_arg(
            alias,
            object_classes,
            owner_class=owner_class,
            field_name=field_name,
            ctx=ctx,
        )
    return _classify_core(
        t,
        object_classes,
        for_return=False,
        ctx=ctx,
        owner_class=owner_class,
        field_name=field_name,
    )


def classify_return(
    t: str,
    object_classes: Dict[str, Class],
    *,
    owner_class: str = "",
    field_name: str = "",
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    n = strip_ref(t)
    if n in ("", "void"):
        return TypeInfo("void", "void", "()")
    info = _classify_core(
        t,
        object_classes,
        for_return=True,
        ctx=ctx,
        owner_class=owner_class,
        field_name=field_name,
    )
    if info and info.kind != "void":
        if info.kind == "object" and not info.lua_type.endswith("?"):
            info = dataclasses.replace(info, lua_type=f"{info.class_name}?")
        elif info.kind == "opaque_handle" and not info.lua_type.endswith("?"):
            info = dataclasses.replace(info, lua_type=f"{info.lua_type}?")
        elif info.kind == "delegate":
            info = dataclasses.replace(info, lua_type=f"{info.lua_type}?")
    return info


def require_classify_arg(
    t: str,
    object_classes: Dict[str, Class],
    *,
    owner_class: str = "",
    ctx: CodegenContext | None = None,
) -> TypeInfo:
    info = classify_arg(t, object_classes, owner_class=owner_class, ctx=ctx)
    if info is None:
        raise ValueError(f"unsupported arg type: {t}")
    return info


def require_classify_return(
    t: str, object_classes: Dict[str, Class], *, ctx: CodegenContext | None = None
) -> TypeInfo:
    info = classify_return(t, object_classes, ctx=ctx)
    if info is None:
        raise ValueError(f"unsupported return type: {t}")
    return info


def method_input_arg_count(
    method,
    object_classes: Dict[str, Class],
    *,
    owner_class: str = "",
    ctx: CodegenContext | None = None,
) -> int:
    from luau_codegen.convert.sel_args import count_lua_method_args

    ret = classify_return(method.ret, object_classes, ctx=ctx)
    if ret is None:
        return len(method.args)
    return count_lua_method_args(method, object_classes, ret.kind, owner_class=owner_class, ctx=ctx)
