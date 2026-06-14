from __future__ import annotations

from dataclasses import dataclass
import os
import re

from luau_codegen.parse.broma import split_top_level
from luau_codegen.parse.text import strip_comments


@dataclass(frozen=True)
class CocosEnumEntry:
    name: str
    value: int


_ENUM_RE = re.compile(
    r"typedef\s+enum\s*(?:[A-Za-z_]\w*)?\s*\{(?P<body>.*?)\}\s*enumKeyCodes\s*;",
    re.DOTALL,
)


def enum_key_codes_header_path(geode_sdk_path: str) -> str:
    return os.path.join(
        geode_sdk_path,
        "loader",
        "include",
        "Geode",
        "cocos",
        "robtop",
        "keyboard_dispatcher",
        "CCKeyboardDelegate.h",
    )


def parse_enum_key_codes(text: str) -> list[CocosEnumEntry]:
    match = _ENUM_RE.search(strip_comments(text))
    if not match:
        return []

    entries: list[CocosEnumEntry] = []
    current = -1
    for raw_part in split_top_level(match.group("body")):
        part = raw_part.strip()
        if not part:
            continue
        name, sep, value_expr = part.partition("=")
        name = name.strip()
        if not re.fullmatch(r"[A-Za-z_]\w*", name):
            continue
        if sep:
            current = int(value_expr.strip(), 0)
        else:
            current += 1
        entries.append(CocosEnumEntry(name=name, value=current))
    return entries


def scan_enum_key_codes(geode_sdk_path: str) -> list[CocosEnumEntry]:
    path = enum_key_codes_header_path(geode_sdk_path)
    if not os.path.exists(path):
        return []
    with open(path, "r", encoding="utf-8") as f:
        return parse_enum_key_codes(f.read())
