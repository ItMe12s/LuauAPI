from __future__ import annotations

from luau_codegen.model.codegen_context import CodegenContext
from luau_codegen.convert.type_map import enum_lua_names


def _enum_block(namespace: str, ctx: CodegenContext) -> str:
    names = sorted(enum_lua_names(namespace, ctx=ctx))
    if not names:
        return ""
    return "".join(f"export type {name} = number\n" for name in names)


def _enum_tables_block(ctx: CodegenContext) -> str:
    entries = ctx.cocos_enum_members.get("enumKeyCodes", ())
    if not entries:
        return ""
    fields = ", ".join(f"{name}: number" for name, _ in entries)
    return f"export type EnumKeyCodesNamespace = {{ {fields} }}\n"
