from __future__ import annotations

import re


def cxx_id(value: str) -> str:
    return re.sub(r"[^A-Za-z0-9_]", "_", value)
