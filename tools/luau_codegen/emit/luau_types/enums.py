from __future__ import annotations

from luau_codegen.convert.type_map import enum_lua_names


def _enum_block(namespace: str) -> str:
    names = sorted(enum_lua_names(namespace))
    if not names:
        return ""
    return "".join(f"export type {name} = number\n" for name in names)
