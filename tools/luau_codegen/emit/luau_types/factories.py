from __future__ import annotations

from typing import TYPE_CHECKING
from collections.abc import Sequence

from luau_codegen.parse.broma import Class, Method

if TYPE_CHECKING:
    from luau_codegen.model.codegen_context import CodegenContext
    from luau_codegen.model.type_analysis import TypeAnalysis
from luau_codegen.model.domain import lua_namespace
from luau_codegen.emit.luau_types.method_types import (
    _method_type,
    _widened_method_type,
    lua_export_name,
)


def _emit_factory_records(
    factories: dict[str, dict[str, list[Method]]],
    objects: dict[str, Class],
    ctx: CodegenContext | None = None,
    analysis: TypeAnalysis | None = None,
) -> list[str]:
    lines: list[str] = []
    for cls_name in sorted(factories):
        methods = factories[cls_name]
        lines.append(f"export type {cls_name}Factory = {{\n")
        for name, overloads in sorted(methods.items()):
            if len(overloads) > 1:
                type_str = _widened_method_type(
                    objects[cls_name], overloads, objects, static=True, ctx=ctx, analysis=analysis
                )
            else:
                type_str = _method_type(
                    objects[cls_name], overloads, objects, ctx=ctx, analysis=analysis
                )
            lines.append(f"    {name}: {type_str},\n")
        lines.append("}\n\n")
    return lines


def _factory_field_lines(factories: dict[str, dict[str, list[Method]]]) -> list[str]:
    return [f"    {cls_name}: {cls_name}Factory,\n" for cls_name in sorted(factories)]


def _emit_factories(
    factories: dict[str, dict[str, list[Method]]],
    objects: dict[str, Class],
    namespace: str,
    ctx: CodegenContext | None = None,
    extra_field_lines: list[str] | None = None,
    analysis: TypeAnalysis | None = None,
) -> list[str]:
    lines = _emit_factory_records(factories, objects, ctx=ctx, analysis=analysis)
    field_lines = _factory_field_lines(factories)
    if extra_field_lines:
        field_lines = sorted(
            field_lines + extra_field_lines,
            key=lambda line: line.strip().split(":", 1)[0].strip(),
        )
    lines.append(f"export type {_namespace_type_name(namespace)} = {{\n")
    lines.extend(field_lines)
    lines.append("}\n\n")
    return lines


def _namespace_type_name(namespace: str) -> str:
    if namespace == "geode.cocos2d":
        return "Cocos2dNamespace"
    return "GDNamespace"


def _collect_factories(
    classes: Sequence[Class],
    grouped_by_class: dict[str, dict[str, list[Method]]],
    skipped_classes: set,
    namespace: str,
) -> dict[str, dict[str, list[Method]]]:
    factories: dict[str, dict[str, list[Method]]] = {}
    for cls in classes:
        if cls.name in skipped_classes or lua_namespace(cls) != namespace:
            continue
        grouped = grouped_by_class[cls.name]
        static_methods: dict[str, list[Method]] = {}
        for cpp_name, methods in grouped.items():
            if not methods[0].is_static:
                continue
            lua_name = lua_export_name(cpp_name, grouped)
            if lua_name is None:
                continue
            static_methods[lua_name] = methods
        if static_methods:
            factories[cls.name] = static_methods
    return factories
