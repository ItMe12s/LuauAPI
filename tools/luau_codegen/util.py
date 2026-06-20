from __future__ import annotations

import re

VALID_PLATFORMS = {
    "win",
    "android",
    "android32",
    "android64",
    "ios",
    "mac",
    "imac",
    "m1",
}

INTERSECTION_PLATFORMS = ("win", "m1", "ios", "android32", "android64")


def cxx_id(value: str) -> str:
    return re.sub(r"[^A-Za-z0-9_]", "_", value)


def binding_filename(cls_name: str) -> str:
    return f"bindings_{cls_name}.cpp"
