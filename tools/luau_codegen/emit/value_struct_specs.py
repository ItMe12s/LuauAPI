from __future__ import annotations

import importlib
import sys
from pathlib import Path
from typing import TYPE_CHECKING

from luau_codegen.cli.io import _write_if_changed
from luau_codegen.convert.marshalling import emit_stack_check, push_value
from luau_codegen.convert.type_map import (
    COMPOSITE_KINDS,
    classify_arg,
    classify_return,
    iter_type_tree,
)
from luau_codegen.model.codegen_context import CodegenContext
from luau_codegen.model.value_types import (
    CocosValueStructDescriptor,
    FieldDescriptor,
    PushFieldDescriptor,
    PushFieldKind,
    ValueTypeSpec,
)

if TYPE_CHECKING:
    from luau_codegen.parse.broma import Field, Root


VALUE_STRUCT_SPECS_MODULE = "luau_codegen.model.value_struct_specs"


_CONTAINER_KINDS = COMPOSITE_KINDS


def _is_int_cxx(cxx: str) -> bool:
    token = cxx.split("::", 1)[-1].strip()
    return token in ("int", "unsigned int", "unsigned", "short", "unsigned short")


def _check_kind_for(info) -> PushFieldKind:
    if info.kind == "bool":
        return "bool"
    if info.kind == "number":
        return "integer" if _is_int_cxx(info.cxx_type) else "number"
    if info.kind == "enum":
        return "enum"
    if info.kind == "string":
        return "string"
    if info.kind == "value":
        return "nested"
    if info.kind == "object":
        return "object_nullable"
    if info.kind == "opaque_handle":
        return "opaque_nullable"
    if info.kind in _CONTAINER_KINDS:
        return "container"
    raise ValueError(f"unsupported value-struct member kind: {info.kind}")


def _member_for_struct(name: str) -> str:
    return f"value.{name}"


def _render_container_check(field: "Field", info) -> tuple[str, ...]:
    lines = emit_stack_check(info, "idx", f"_{field.name}_tmp", field.name)
    lines.append(
        f"        luax::assignContainerValue(value.{field.name}, std::move(_{field.name}_tmp));\n"
    )
    return tuple(lines)


def _render_container_push(field: "Field", info) -> tuple[str, ...]:
    lines = push_value(info, f"value.{field.name}")
    lines.append(f'        lua_setfield(L, -2, "{field.name}");\n')
    return tuple(lines)


def _stub_line(field: "Field", ret_info) -> str:
    lua = ret_info.lua_type
    return f"    {field.name}: {lua}"


def _stub_dep_for(ret_info) -> str | None:
    if ret_info.kind == "value":
        return ret_info.lua_type
    if ret_info.kind == "object":
        return ret_info.class_name
    if ret_info.kind == "opaque_handle":
        return ret_info.lua_type.removesuffix("?")
    return None


def _stub_deps_for_field(ret_info) -> set[str]:
    return {dep for node in iter_type_tree(ret_info) if (dep := _stub_dep_for(node)) is not None}


def _build_spec(
    cls_name: str,
    fields: list["Field"],
    ctx: CodegenContext,
    objects: dict,
) -> ValueTypeSpec | None:
    check_fields: list[FieldDescriptor] = []
    push_fields: list[PushFieldDescriptor] = []
    stub_lines: list[str] = []
    deps: set[str] = set()

    for field in fields:
        if field.access != "public" or field.count > 1:
            continue
        arg_info = classify_arg(field.type, objects, ctx=ctx)
        ret_info = classify_return(field.type, objects, ctx=ctx)
        if arg_info is None or ret_info is None:
            continue
        try:
            check_kind = _check_kind_for(arg_info)
        except ValueError:
            continue
        member = _member_for_struct(field.name)
        deps.update(_stub_deps_for_field(ret_info))

        if check_kind == "container":
            c_override = _render_container_check(field, arg_info)
            p_override = _render_container_push(field, arg_info)
            check_fields.append(
                FieldDescriptor(field.name, "container", member, check_override=c_override)
            )
            push_fields.append(
                PushFieldDescriptor(field.name, "container", member, push_override=p_override)
            )
        elif check_kind == "nested":
            nested_cxx = ret_info.cxx_type
            check_fields.append(
                FieldDescriptor(field.name, "nested", member, nested_type=nested_cxx)
            )
            push_fields.append(
                PushFieldDescriptor(field.name, "nested", member, nested_type=nested_cxx)
            )
        elif check_kind in ("object_nullable", "opaque_nullable"):
            pointee = (
                arg_info.cxx_type[:-1] if arg_info.cxx_type.endswith("*") else arg_info.cxx_type
            )
            check_fields.append(FieldDescriptor(field.name, check_kind, member, cxx_type=pointee))
            push_fields.append(
                PushFieldDescriptor(field.name, check_kind, member, cxx_type=pointee)
            )
        elif check_kind == "enum":
            enum_cxx = ret_info.cxx_type
            check_fields.append(FieldDescriptor(field.name, "enum", member, cxx_type=enum_cxx))
            push_fields.append(PushFieldDescriptor(field.name, "enum", member, cxx_type=enum_cxx))
        elif check_kind == "string":
            store = "gd::string" if arg_info.cxx_type in ("gd::string",) else "std::string"
            check_fields.append(FieldDescriptor(field.name, "string", member, cxx_type=store))
            push_fields.append(PushFieldDescriptor(field.name, "string", member))
        else:
            check_fields.append(FieldDescriptor(field.name, check_kind, member))
            push_fields.append(PushFieldDescriptor(field.name, check_kind, member))

        stub_lines.append(_stub_line(field, ret_info))

    if not check_fields:
        return None

    cxx_type = cls_name
    luau_stub = "export type " + cls_name + " = {\n" + ",\n".join(stub_lines) + ",\n}\n"
    return ValueTypeSpec(
        lua_name=cls_name,
        cxx_type=cxx_type,
        cxx_aliases=(cls_name,),
        luau_stub=luau_stub,
        stub_deps=tuple(sorted(deps)),
        cocos=CocosValueStructDescriptor(
            cxx_type=cxx_type,
            check_fields=tuple(check_fields),
            push_fields=tuple(push_fields),
            push_capacity=len(push_fields),
        ),
    )


def collect_value_struct_specs(root: "Root", platform: str = "win") -> tuple[ValueTypeSpec, ...]:
    from luau_codegen.model.value_struct_gate import VALUE_STRUCT_OPT_IN

    ctx = root.codegen_ctx or CodegenContext.static()
    objects = {c.name: c for c in root.classes}
    opt_in = set(VALUE_STRUCT_OPT_IN)

    from luau_codegen.convert import type_map as _type_map

    provisional = dict(_type_map.VALUE_TYPES)
    for name in opt_in:
        provisional.setdefault(name, name)
    _type_map.VALUE_TYPES = provisional
    from luau_codegen.convert import type_classification as _tc

    _tc.VALUE_TYPES = provisional

    specs: list[ValueTypeSpec] = []
    seen: set[str] = set()
    try:
        for name in VALUE_STRUCT_OPT_IN:
            if name in seen:
                continue
            cls = objects.get(name)
            if cls is None:
                root.scan_warnings.append(
                    f"value_struct_specs: opt-in '{name}' not found in parsed classes"
                )
                continue
            seen.add(name)
            spec = _build_spec(cls.name, cls.fields, ctx, objects)
            if spec is None:
                root.scan_warnings.append(
                    f"value_struct_specs: '{name}' has no bindable public members"
                )
                continue
            specs.append(spec)
    finally:
        from luau_codegen.model import value_types as _vt

        _vt._rebuild_value_type_maps()
    return tuple(specs)


def emit_specs_py(specs: tuple[ValueTypeSpec, ...]) -> str:
    lines = [
        "# Generated by luauapi_codegen value_struct_specs. Do not edit by hand.",
        "# Re-derived from Broma bindings on each codegen run.",
        "from luau_codegen.model.value_types import (",
        "    CocosValueStructDescriptor,",
        "    FieldDescriptor,",
        "    PushFieldDescriptor,",
        "    ValueTypeSpec,",
        ")",
        "",
        "",
        "VALUE_STRUCT_SPECS: tuple[ValueTypeSpec, ...] = (",
    ]
    for spec in specs:
        lines.append(repr(spec) + ",")
    lines.append(")")
    lines.append("")
    return "\n".join(lines)


def install_value_struct_specs_module(
    specs: tuple[ValueTypeSpec, ...],
    *,
    specs_path: Path | str | None = None,
    module_name: str = VALUE_STRUCT_SPECS_MODULE,
    preserve_existing_on_empty: bool = False,
) -> None:
    mod = sys.modules.get(module_name)
    if mod is None:
        try:
            mod = importlib.import_module(module_name)
        except ImportError:
            mod = None
    if preserve_existing_on_empty and not specs:
        if mod is not None and getattr(mod, "VALUE_STRUCT_SPECS", None):
            return
    if mod is not None:
        if specs_path is not None:
            mod.__file__ = str(specs_path)
        setattr(mod, "VALUE_STRUCT_SPECS", specs)
    from luau_codegen.model.value_types import _rebuild_value_type_maps

    _rebuild_value_type_maps()


def emit_value_struct_artifacts(
    root: "Root",
    *,
    specs_out: Path | str | None = None,
    install_module: bool = True,
    preserve_existing_on_empty: bool = False,
) -> tuple[ValueTypeSpec, ...]:
    specs = collect_value_struct_specs(root)
    if specs_out is not None:
        _write_if_changed(str(specs_out), emit_specs_py(specs))
    if install_module:
        install_value_struct_specs_module(
            specs,
            specs_path=specs_out,
            preserve_existing_on_empty=preserve_existing_on_empty,
        )
    return specs
