from __future__ import annotations

from typing import Dict, Optional

from luau_codegen.convert import type_containers as containers
from luau_codegen.convert.type_map import (
    CALLBACK_ALIASES,
    CLASS_CALLBACK_ALIASES,
    NUMERIC_TYPES,
    OPAQUE_HANDLE_TYPES,
    SEL_TYPES,
    STRING_TYPES,
    VALUE_TYPES,
    WIDE_INTEGER_TYPES,
    TypeInfo,
    cxx_class_name,
    enum_cxx_type,
    resolve_object_class,
    sel_lua_type,
    sel_variant,
)
from luau_codegen.convert.type_normalization import (
    callback_inner,
    is_out_reference,
    is_reference_type,
    normalize_type,
    strip_ref,
    without_pointer,
)
from luau_codegen.model import delegate_specs as _delegate_specs
from luau_codegen.model.codegen_context import CodegenContext
from luau_codegen.model.domain import short_name
from luau_codegen.parse.broma import Class, split_arg, split_top_level


def _resolve_ctx(ctx: CodegenContext | None) -> CodegenContext:
    if ctx is not None:
        return ctx
    return CodegenContext.static()


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
    if ret_info is None:
        return None
    arg_infos = []
    for raw in split_top_level(inner[paren + 1 : close]):
        raw = raw.strip()
        if not raw:
            continue
        parsed = split_arg(raw)
        info = classify_arg(parsed.type, object_classes, ctx=ctx)
        if info is None or info.kind == "callback":
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
    base = short_name(without_pointer(n)) if n.endswith("*") else short_name(n)

    if n.endswith("*"):
        base = n[:-1].strip()
        ptr_container = containers.parse_container(
            n[:-1].strip(), object_classes, TypeInfo, ctx=ctx
        )
        if ptr_container is not None:
            return containers.with_container_ref_flags(
                ptr_container,
                TypeInfo,
                is_ref=False,
                is_out=True,
                is_vector_ptr=True,
            )
        ptr_primitive = containers.parse_primitive_vector(base, object_classes, TypeInfo, ctx=ctx)
        if ptr_primitive is not None:
            return TypeInfo(
                ptr_primitive.kind,
                ptr_primitive.cxx_type,
                ptr_primitive.lua_type,
                ptr_primitive.class_name,
                is_ref=False,
                is_out=True,
                is_vector_ptr=True,
                element_type=ptr_primitive.element_type,
            )
        ptr_std_array = containers.parse_std_array(base, object_classes, TypeInfo, ctx=ctx)
        if ptr_std_array is not None:
            return TypeInfo(
                ptr_std_array.kind,
                ptr_std_array.cxx_type,
                ptr_std_array.lua_type,
                ptr_std_array.class_name,
                is_ref=False,
                is_out=True,
                is_vector_ptr=True,
                element_type=ptr_std_array.element_type,
                array_size=ptr_std_array.array_size,
            )
        ptr_nested = containers.parse_nested_primitive_vector_view(
            base, object_classes, TypeInfo, ctx=ctx
        )
        if ptr_nested is not None:
            return TypeInfo(
                ptr_nested.kind,
                ptr_nested.cxx_type,
                ptr_nested.lua_type,
                ptr_nested.class_name,
                is_ref=False,
                is_out=True,
                is_vector_ptr=True,
                element_type=ptr_nested.element_type,
            )
        ptr_vector = containers.parse_vector_view(base, object_classes, TypeInfo, ctx=ctx)
        if ptr_vector is not None:
            return TypeInfo(
                ptr_vector.kind,
                ptr_vector.cxx_type,
                ptr_vector.lua_type,
                ptr_vector.class_name,
                is_ref=False,
                is_out=True,
                is_vector_ptr=True,
                element_type=ptr_vector.element_type,
            )

    std_array = containers.parse_std_array(n, object_classes, TypeInfo, ctx=ctx)
    if std_array is not None:
        return TypeInfo(
            std_array.kind,
            std_array.cxx_type,
            std_array.lua_type,
            std_array.class_name,
            is_ref,
            is_out,
            element_type=std_array.element_type,
            array_size=std_array.array_size,
        )

    nested_primitive_vector_view = containers.parse_nested_primitive_vector_view(
        n, object_classes, TypeInfo, ctx=ctx
    )
    if nested_primitive_vector_view is not None:
        return TypeInfo(
            nested_primitive_vector_view.kind,
            nested_primitive_vector_view.cxx_type,
            nested_primitive_vector_view.lua_type,
            nested_primitive_vector_view.class_name,
            is_ref,
            is_out,
            element_type=nested_primitive_vector_view.element_type,
        )

    primitive_vector = containers.parse_primitive_vector(n, object_classes, TypeInfo, ctx=ctx)
    if primitive_vector is not None:
        return TypeInfo(
            primitive_vector.kind,
            primitive_vector.cxx_type,
            primitive_vector.lua_type,
            primitive_vector.class_name,
            is_ref,
            is_out,
            element_type=primitive_vector.element_type,
        )

    vector_view = containers.parse_vector_view(n, object_classes, TypeInfo, ctx=ctx)
    if vector_view is not None:
        return TypeInfo(
            vector_view.kind,
            vector_view.cxx_type,
            vector_view.lua_type,
            vector_view.class_name,
            is_ref,
            is_out,
            element_type=vector_view.element_type,
        )

    cc_c_array_view = containers.parse_cc_c_array_view(
        n, owner_class, field_name, object_classes, TypeInfo, ctx=ctx
    )
    if cc_c_array_view is not None:
        return TypeInfo(
            cc_c_array_view.kind,
            cc_c_array_view.cxx_type,
            cc_c_array_view.lua_type,
            cc_c_array_view.class_name,
            is_ref,
            is_out,
            element_type=cc_c_array_view.element_type,
        )

    container = containers.parse_container(n, object_classes, TypeInfo, ctx=ctx)
    if container is not None:
        return containers.with_container_ref_flags(
            container,
            TypeInfo,
            is_ref=is_ref,
            is_out=is_out,
        )
    pair_type = containers.parse_std_pair(n, object_classes, TypeInfo, ctx=ctx)
    if pair_type is not None:
        return TypeInfo(
            pair_type.kind,
            pair_type.cxx_type,
            pair_type.lua_type,
            pair_type.class_name,
            is_ref=is_ref,
            is_out=is_out,
            key_type=pair_type.key_type,
            value_type=pair_type.value_type,
        )
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
            info = TypeInfo(
                info.kind,
                info.cxx_type,
                f"{info.class_name}?",
                info.class_name,
                info.is_ref,
                info.is_out,
                info.is_vector_ptr,
            )
        elif info.kind == "opaque_handle" and not info.lua_type.endswith("?"):
            info = TypeInfo(
                info.kind,
                info.cxx_type,
                f"{info.lua_type}?",
                is_ref=info.is_ref,
                is_out=info.is_out,
                is_vector_ptr=info.is_vector_ptr,
            )
        elif info.kind == "delegate":
            info = TypeInfo(
                info.kind,
                info.cxx_type,
                f"{info.lua_type}?",
                info.class_name,
                info.is_ref,
                info.is_out,
                info.is_vector_ptr,
            )
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
