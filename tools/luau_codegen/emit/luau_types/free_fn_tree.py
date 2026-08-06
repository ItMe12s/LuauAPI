from __future__ import annotations

from typing import TYPE_CHECKING

from luau_codegen.parse.broma import Class, Function, Method

if TYPE_CHECKING:
    from luau_codegen.model.codegen_context import CodegenContext
    from luau_codegen.model.type_analysis import TypeAnalysis
from luau_codegen.emit.luau_types.method_types import (
    _DUMMY_CLS,
    _method_type,
    _widened_method_type,
)


def _new_node() -> dict:
    return {"children": {}, "functions": {}, "manual": []}


def _build_function_tree(
    functions: list[Function],
    objects: dict[str, Class],
    manual_fields: dict[str, list[str]] | None = None,
) -> dict:
    tree: dict = _new_node()
    for fn in functions:
        node = tree
        for seg in fn.lua_path.split(".")[1:]:
            node = node["children"].setdefault(seg, _new_node())
        node["functions"].setdefault(fn.name, []).append(fn)

    for lua_path, fields in (manual_fields or {}).items():
        node = tree
        for seg in lua_path.split(".")[1:]:
            node = node["children"].setdefault(seg, _new_node())
        node["manual"].extend(fields)
    return tree


def _emit_function_tree(
    node: dict,
    objects: dict[str, Class],
    indent: int,
    ctx: CodegenContext | None = None,
    analysis: TypeAnalysis | None = None,
) -> list[str]:
    pad = "    " * indent
    lines: list[str] = []
    manual_names = {field.split(":", 1)[0].strip() for field in node.get("manual", [])}
    for name in sorted(node["functions"]):
        if name in manual_names:
            continue
        methods = [
            Method(name=fn.name, ret=fn.ret, args=fn.args, is_static=True)
            for fn in node["functions"][name]
        ]
        if len(methods) > 1:
            type_str = _widened_method_type(
                _DUMMY_CLS, methods, objects, static=True, ctx=ctx, analysis=analysis
            )
        else:
            type_str = _method_type(_DUMMY_CLS, methods, objects, ctx=ctx, analysis=analysis)
        lines.append(f"{pad}{name}: {type_str},\n")
    for field in sorted(node.get("manual", [])):
        lines.append(f"{pad}{field},\n")
    for seg in sorted(node["children"]):
        lines.append(f"{pad}{seg}: {{\n")
        lines.extend(
            _emit_function_tree(
                node["children"][seg], objects, indent + 1, ctx=ctx, analysis=analysis
            )
        )
        lines.append(f"{pad}}},\n")
    return lines
