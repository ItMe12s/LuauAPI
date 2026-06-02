from __future__ import annotations

from typing import TYPE_CHECKING, Dict, List, Sequence

from luau_codegen.parse.broma import Class, Method

if TYPE_CHECKING:
    from luau_codegen.model.codegen_context import CodegenContext
from luau_codegen.model.domain import lua_namespace
from luau_codegen.emit.luau_types.method_types import (
    LUAU_KEYWORDS,
    _method_type,
    _widened_method_type,
)


def _emit_factory_records(
    factories: Dict[str, Dict[str, List[Method]]],
    objects: Dict[str, Class],
    ctx: CodegenContext | None = None,
) -> List[str]:
    lines: List[str] = []
    for cls_name in sorted(factories):
        methods = factories[cls_name]
        lines.append(f"export type {cls_name}Factory = {{\n")
        for name, overloads in sorted(methods.items()):
            if len(overloads) > 1:
                type_str = _widened_method_type(
                    objects[cls_name], overloads, objects, static=True, ctx=ctx
                )
            else:
                type_str = _method_type(objects[cls_name], overloads, objects, ctx=ctx)
            lines.append(f"    {name}: {type_str},\n")
        lines.append("}\n\n")
    return lines


def _factory_field_lines(factories: Dict[str, Dict[str, List[Method]]]) -> List[str]:
    return [f"    {cls_name}: {cls_name}Factory,\n" for cls_name in sorted(factories)]


def _emit_factories(
    factories: Dict[str, Dict[str, List[Method]]],
    objects: Dict[str, Class],
    namespace: str,
    ctx: CodegenContext | None = None,
) -> List[str]:
    lines = _emit_factory_records(factories, objects, ctx=ctx)
    lines.append(f"export type {_namespace_type_name(namespace)} = {{\n")
    lines.extend(_factory_field_lines(factories))
    lines.append("}\n\n")
    return lines


def _namespace_type_name(namespace: str) -> str:
    if namespace == "geode.cocos2d":
        return "Cocos2dNamespace"
    return "GDNamespace"


def _collect_factories(
    classes: Sequence[Class],
    grouped_by_class: Dict[str, Dict[str, List[Method]]],
    skipped_classes: set,
    namespace: str,
) -> Dict[str, Dict[str, List[Method]]]:
    factories: Dict[str, Dict[str, List[Method]]] = {}
    for cls in classes:
        if cls.name in skipped_classes or lua_namespace(cls) != namespace:
            continue
        grouped = grouped_by_class[cls.name]
        static_methods = {
            k: v
            for k, v in grouped.items()
            if v[0].is_static and k not in LUAU_KEYWORDS
        }
        if static_methods:
            factories[cls.name] = static_methods
    return factories
