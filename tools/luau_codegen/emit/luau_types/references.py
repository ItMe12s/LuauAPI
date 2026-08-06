from __future__ import annotations

import re
from typing import TYPE_CHECKING
from collections.abc import Sequence

if TYPE_CHECKING:
    from luau_codegen.model.codegen_context import CodegenContext
    from luau_codegen.model.type_analysis import TypeAnalysis

from luau_codegen.parse.broma import Class, Field, Function, Method
from luau_codegen.policy.fields import bindable_field
from luau_codegen.model.domain import short_name
from luau_codegen.convert.type_primitives import TypeInfo, iter_type_tree
from luau_codegen.convert.type_classification import classify_arg, classify_return
from luau_codegen.model.delegate_specs import DelegateCatalog
from luau_codegen.model.value_types import ValueTypeCatalog

_OPAQUE_STUB_BODY: dict[str, str] = {
    "FMODChannel": "--- @type-only: opaque FMOD handle\ndeclare class FMODChannel end\n\n",
    "FMODSoundHandle": (
        "--- @type-only: opaque FMOD handle\ndeclare class FMODSoundHandle end\n\n"
    ),
    "FMODSystem": "--- @type-only: opaque FMOD handle\ndeclare class FMODSystem end\n\n",
    "FMODDSP": "--- @type-only: opaque FMOD handle\ndeclare class FMODDSP end\n\n",
    "FMODChannelGroup": (
        "--- @type-only: opaque FMOD handle\ndeclare class FMODChannelGroup end\n\n"
    ),
    "CCEvent": "--- @type-only: opaque cocos2d handle\ndeclare class CCEvent end\n\n",
    "CCEditBox": ("--- @type-only: opaque cocos2d handle\ndeclare class CCEditBox end\n\n"),
    "GroupCommandObject2": (
        "--- @type-only: non-CCObject GD type\ndeclare class GroupCommandObject2 end\n\n"
    ),
    "DelayedSpawnNode": (
        "--- @type-only: non-CCObject GD type\ndeclare class DelayedSpawnNode end\n\n"
    ),
}

_OPAQUE_STUB_ORDER = (
    "FMODChannel",
    "FMODSoundHandle",
    "FMODSystem",
    "FMODDSP",
    "FMODChannelGroup",
    "CCEvent",
    "CCEditBox",
    "GroupCommandObject2",
    "DelayedSpawnNode",
)


def _expand_value_refs(names: set[str], catalog: ValueTypeCatalog) -> set[str]:
    out: set[str] = set(names)
    queue = list(names)
    while queue:
        name = queue.pop()
        for dep in catalog.stub_deps.get(name, ()):
            if dep not in out:
                out.add(dep)
                queue.append(dep)
    return {n for n in out if n in catalog.stub_body}


def _value_type_object_refs(info: TypeInfo, catalog: ValueTypeCatalog) -> set[str]:
    if info.kind != "value":
        return set()
    return set(catalog.stub_deps.get(info.lua_type, ()))


def _all_type_stub_bodies(catalog: ValueTypeCatalog) -> dict[str, str]:
    return {**catalog.stub_body, **_OPAQUE_STUB_BODY}


def _value_refs_in_text(text: str, catalog: ValueTypeCatalog) -> set[str]:
    bodies = _all_type_stub_bodies(catalog)
    return {name for name in bodies if re.search(rf"\b{name}\b", text)}


def _emit_value_stub_block(names: set[str], catalog: ValueTypeCatalog) -> str:
    expanded = _expand_value_refs(names, catalog)
    expanded |= {n for n in names if n in _OPAQUE_STUB_BODY}
    if not expanded:
        return ""
    bodies = _all_type_stub_bodies(catalog)

    order = catalog.stub_order + _OPAQUE_STUB_ORDER
    ordered = list(order) + sorted(expanded - set(order))
    parts: list[str] = []
    prev_was_value = False
    for name in ordered:
        if name not in expanded:
            continue
        is_opaque = name in _OPAQUE_STUB_BODY
        if is_opaque and prev_was_value:
            parts.append("\n")
        parts.append(bodies[name])
        prev_was_value = name in catalog.stub_body
    return "".join(parts)


def _object_type_name(info: TypeInfo) -> str:
    if info.class_name:
        return info.class_name
    return info.lua_type.removesuffix("?")


def _emit_delegate_stub_block(catalog: DelegateCatalog) -> str:
    if not catalog.specs:
        return ""
    lines = ["-- Delegate table types\n\n"]
    seen: set[str] = set()
    for spec in catalog.specs.values():
        if spec.lua_name in seen:
            continue
        seen.add(spec.lua_name)
        fields = []
        for m in spec.methods:
            params = ", ".join(f"arg{i}: {t}" for i, t in enumerate(m.args_lua, start=1))
            ret = m.ret_lua
            fn = f"({params}) -> ()" if ret == "()" else f"({params}) -> {ret}"
            fields.append(f"    {m.name}: ({fn})?")
        body = ",\n".join(fields)
        lines.append(f"export type {spec.lua_name} = {{\n{body}\n}}\n\n")
    return "".join(lines)


def _emit_generated_support_stub_block() -> str:
    return (
        "export type GeodeTaskHandle<T> = {\n"
        "    onComplete: (self: GeodeTaskHandle<T>, callback: (value: T?, err: string?) -> ()) -> GeodeTaskHandle<T>,\n"
        "    cancel: (self: GeodeTaskHandle<T>) -> (),\n"
        "    detach: (self: GeodeTaskHandle<T>) -> (),\n"
        "    isPending: (self: GeodeTaskHandle<T>) -> boolean,\n"
        "    isDone: (self: GeodeTaskHandle<T>) -> boolean,\n"
        "    isDetached: (self: GeodeTaskHandle<T>) -> boolean,\n"
        "}\n\n"
    )


def _refs_from_type(info: TypeInfo, catalog: ValueTypeCatalog) -> set[str]:
    refs: set[str] = set()
    for node in iter_type_tree(info):
        if node.kind == "value":
            refs.add(node.lua_type)
            refs.update(_value_type_object_refs(node, catalog))
        elif node.kind == "object":
            refs.add(_object_type_name(node))
        elif node.kind == "opaque_handle":
            refs.add(node.lua_type.removesuffix("?"))
    return refs


def _value_catalog(ctx: CodegenContext | None, analysis: TypeAnalysis | None) -> ValueTypeCatalog:
    if ctx is not None:
        return ctx.value_types
    if analysis is not None:
        return analysis.ctx.value_types
    return ValueTypeCatalog.builtins()


def _refs_from_method(
    method: Method,
    objects: dict[str, Class],
    ctx: CodegenContext | None = None,
    analysis: TypeAnalysis | None = None,
    owner_class: str = "",
) -> set[str]:
    refs: set[str] = set()
    catalog = _value_catalog(ctx, analysis)
    for arg in method.args:
        info = (
            analysis.classify_arg(arg.type, owner_class=owner_class)
            if analysis
            else classify_arg(arg.type, objects, ctx=ctx)
        )
        if info:
            refs.update(_refs_from_type(info, catalog))
    ret = (
        analysis.classify_return(method.ret, owner_class=owner_class)
        if analysis
        else classify_return(method.ret, objects, ctx=ctx)
    )
    if ret:
        refs.update(_refs_from_type(ret, catalog))
    return refs


def _refs_from_functions(
    functions: Sequence[Function],
    objects: dict[str, Class],
    ctx: CodegenContext | None = None,
    analysis: TypeAnalysis | None = None,
) -> set[str]:
    refs: set[str] = set()
    catalog = _value_catalog(ctx, analysis)
    for function in functions:
        for arg in function.args:
            info = (
                analysis.classify_arg(arg.type)
                if analysis
                else classify_arg(arg.type, objects, ctx=ctx)
            )
            if info:
                refs.update(_refs_from_type(info, catalog))
        ret = (
            analysis.classify_return(function.ret)
            if analysis
            else classify_return(function.ret, objects, ctx=ctx)
        )
        if ret:
            refs.update(_refs_from_type(ret, catalog))
    return refs


def _refs_from_fields(
    cls: Class,
    fields: Sequence[Field],
    objects: dict[str, Class],
    ctx: CodegenContext | None = None,
    analysis: TypeAnalysis | None = None,
) -> set[str]:
    refs: set[str] = set()
    catalog = _value_catalog(ctx, analysis)
    for field in fields:
        ok, _, _, ret = bindable_field(field, objects, cls, ctx=ctx, analysis=analysis)
        if ok and ret:
            refs.update(_refs_from_type(ret, catalog))
    return refs


def _base_type_refs(cls: Class, objects: dict[str, Class], skipped_classes: set) -> set[str]:
    refs: set[str] = set()
    for base in cls.bases:
        base_cls = objects.get(short_name(base))
        if base_cls and base_cls.name not in skipped_classes:
            refs.add(base_cls.name)
    return refs


def _refs_from_classes(
    class_names: set[str],
    grouped_by_class: dict[str, dict[str, list[Method]]],
    objects: dict[str, Class],
    skipped_classes: set,
    ctx: CodegenContext | None = None,
    analysis: TypeAnalysis | None = None,
) -> set[str]:
    refs: set[str] = set()
    for name in class_names:
        cls = objects.get(name)
        if not cls or name in skipped_classes:
            continue
        refs.update(_base_type_refs(cls, objects, skipped_classes))
        refs.update(_refs_from_fields(cls, cls.fields, objects, ctx=ctx, analysis=analysis))
        for methods in grouped_by_class.get(name, {}).values():
            for method in methods:
                refs.update(
                    _refs_from_method(
                        method, objects, ctx=ctx, analysis=analysis, owner_class=cls.name
                    )
                )
    return refs


def _factory_object_refs(
    factories: dict[str, dict[str, list[Method]]],
    objects: dict[str, Class],
    ctx: CodegenContext | None = None,
    analysis: TypeAnalysis | None = None,
) -> set[str]:
    refs: set[str] = set()
    for methods in factories.values():
        for overloads in methods.values():
            for method in overloads:
                refs.update(_refs_from_method(method, objects, ctx=ctx, analysis=analysis))
    return refs


def _refs_from_text(content: str) -> set[str]:
    refs: set[str] = set()
    for match in re.finditer(r"extends (\w+)", content):
        refs.add(match.group(1))
    for match in re.finditer(r":\s*(\w+)\??(?:\s*[,&)]|\s*$|\s*->)", content, re.MULTILINE):
        name = match.group(1)
        if name[0].isupper():
            refs.add(name)
    for match in re.finditer(r"<\s*(\w+)\??\s*>", content):
        name = match.group(1)
        if name[0].isupper():
            refs.add(name)
    return refs


def _emit_orphan_stubs(names: set[str]) -> str:
    if not names:
        return ""
    lines = [
        "-- Forward declarations for referenced classes without bindable members\n",
        "-- @type-only: opaque handles, no members bound on this platform\n",
        "\n",
    ]
    for name in sorted(names):
        lines.append("--- @type-only\n")
        lines.append(f"declare class {name} end\n\n")
    return "".join(lines)
