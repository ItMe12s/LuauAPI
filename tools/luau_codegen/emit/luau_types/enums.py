from __future__ import annotations

from typing import Mapping, Tuple

from luau_codegen.model.codegen_context import CodegenContext
from luau_codegen.convert.type_map import enum_lua_names


def enum_namespace_type_name(enum_name: str) -> str:
    if enum_name == "enumKeyCodes":
        return "EnumKeyCodesNamespace"
    return f"{enum_name}Namespace"


def enum_namespace_field_lines(
    members: Mapping[str, Tuple[Tuple[str, int], ...]],
    indent: str = "    ",
) -> list[str]:
    lines: list[str] = []
    for enum_name in sorted(members):
        entries = members[enum_name]
        if not entries:
            continue
        ns_type = enum_namespace_type_name(enum_name)
        lines.append(f"{indent}{enum_name}: {ns_type},\n")
    return lines


def _enum_block(namespace: str, ctx: CodegenContext) -> str:
    names = sorted(enum_lua_names(namespace, ctx=ctx))
    if not names:
        return ""
    return "".join(f"export type {name} = number\n" for name in names)


def _emit_enum_namespace_type(enum_name: str, entries: Tuple[Tuple[str, int], ...]) -> str:
    fields = ", ".join(f"{name}: number" for name, _ in entries)
    ns_type = enum_namespace_type_name(enum_name)
    return f"export type {ns_type} = {{ {fields} }}\n"


def _enum_tables_block(ctx: CodegenContext) -> str:
    seen: set[str] = set()
    blocks: list[str] = []
    for members in (ctx.cocos_enum_members, ctx.gd_enum_members, ctx.geode_enum_members):
        for enum_name in sorted(members):
            if enum_name in seen:
                continue
            entries = members[enum_name]
            if not entries:
                continue
            seen.add(enum_name)
            blocks.append(_emit_enum_namespace_type(enum_name, entries))
    return "".join(blocks)
