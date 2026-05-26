from __future__ import annotations

import re

from broma_parser import Class

_LINK_ATTR_RE = re.compile(r"link\(([^)]*)\)")


def platform_aliases(target_platform: str) -> set[str]:
    return {
        "mac": {"mac", "imac", "m1"},
        "imac": {"mac", "imac"},
        "m1": {"mac", "m1"},
        "android": {"android", "android32", "android64"},
        "android32": {"android", "android32"},
        "android64": {"android", "android64"},
    }.get(target_platform, {target_platform})


def class_link_platforms(cls: Class) -> set[str]:
    out: set[str] = set()
    for attr in cls.attributes:
        match = _LINK_ATTR_RE.search(attr)
        if not match:
            continue
        out.update(part.strip() for part in match.group(1).split(",") if part.strip())
    return out


def is_link_platform(cls: Class, target_platform: str) -> bool:
    return bool(class_link_platforms(cls) & platform_aliases(target_platform))
