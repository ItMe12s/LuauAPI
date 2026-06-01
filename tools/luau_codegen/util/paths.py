from __future__ import annotations


def binding_filename(cls_name: str) -> str:
    return f"bindings_{cls_name}.cpp"
