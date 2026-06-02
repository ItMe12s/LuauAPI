from __future__ import annotations

from typing import TYPE_CHECKING, Dict, List

from luau_codegen.parse.broma import Class, Function, Method

if TYPE_CHECKING:
    from luau_codegen.model.codegen_context import CodegenContext
from luau_codegen.emit.luau_types.method_types import _DUMMY_CLS, _method_type


def _build_function_tree(
    functions: List[Function],
    objects: Dict[str, Class],
) -> dict:
    tree: dict = {"children": {}, "functions": {}}
    for fn in functions:
        node = tree
        for seg in fn.lua_path.split(".")[1:]:
            node = node["children"].setdefault(seg, {"children": {}, "functions": {}})
        node["functions"].setdefault(fn.name, []).append(fn)
    return tree


def _emit_function_tree(
    node: dict,
    objects: Dict[str, Class],
    indent: int,
    ctx: CodegenContext | None = None,
) -> List[str]:
    pad = "    " * indent
    lines: List[str] = []
    for name in sorted(node["functions"]):
        methods = [
            Method(name=fn.name, ret=fn.ret, args=fn.args, is_static=True)
            for fn in node["functions"][name]
        ]
        lines.append(
            f"{pad}{name}: {_method_type(_DUMMY_CLS, methods, objects, ctx=ctx)},\n"
        )
    for seg in sorted(node["children"]):
        lines.append(f"{pad}{seg}: {{\n")
        lines.extend(
            _emit_function_tree(node["children"][seg], objects, indent + 1, ctx=ctx)
        )
        lines.append(f"{pad}}},\n")
    return lines
