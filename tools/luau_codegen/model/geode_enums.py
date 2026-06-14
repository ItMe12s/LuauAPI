from __future__ import annotations

import re
from dataclasses import dataclass

from luau_codegen.parse.broma import split_top_level

_MEMBER_NAME = re.compile(r"^[A-Za-z_]\w*$")
_INT_LITERAL = re.compile(r"-?0[xX][0-9a-fA-F]+|-?0[bB][01]+|-?[0-9]+")


@dataclass(frozen=True)
class EnumMember:
    name: str
    value: int


@dataclass(frozen=True)
class EnumInfo:
    name: str
    cxx_name: str
    members: tuple[EnumMember, ...] = ()


def parse_enum_members(body: str) -> tuple[tuple[EnumMember, ...], str | None]:
    members: list[EnumMember] = []
    current = -1
    for raw_part in split_top_level(body):
        part = raw_part.strip()
        if not part:
            continue
        name, sep, value_expr = part.partition("=")
        name = name.strip()
        if not _MEMBER_NAME.fullmatch(name):
            continue
        if sep:
            expr = value_expr.strip().rstrip(",")
            if not _INT_LITERAL.fullmatch(expr):
                return (), f"unsupported enum value expression: {expr!r}"
            current = int(expr, 0)
        else:
            current += 1
        members.append(EnumMember(name=name, value=current))
    return tuple(members), None


def enum_names_to_cxx(enums: dict[str, EnumInfo]) -> dict[str, str]:
    return {name: info.cxx_name for name, info in enums.items()}
