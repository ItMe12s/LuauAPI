from __future__ import annotations

import re
from typing import Dict

from broma_parser import Class, Field
from denylist import INACCESSIBLE_CLASSES
from type_map import TypeInfo, classify_arg, classify_return, normalize_type


INACCESSIBLE_FIELDS = {
    ("CCObject", "m_nChildIndex"),
}


def field_key(cls: Class, field: Field) -> str:
    return f"{cls.qualified_name}.{field.name}:{field.type}"


def _is_function_pointer(t: str) -> bool:
    return bool(re.search(r"\(\s*\*", normalize_type(t)))


def bindable_field(
    field: Field, objects: Dict[str, Class], cls: Class | None = None
) -> tuple[bool, str, TypeInfo | None, TypeInfo | None]:
    normalized = normalize_type(field.type)
    if cls and (cls.name, field.name) in INACCESSIBLE_FIELDS:
        return False, "inaccessible-field", None, None
    if field.count > 1:
        return False, "array", None, None
    if "&" in normalized:
        return False, "reference", None, None
    if _is_function_pointer(normalized):
        return False, "function-pointer", None, None
    arg = classify_arg(field.type, objects)
    ret = classify_return(field.type, objects)
    if arg is None:
        return False, f"unsupported-arg:{field.type}", None, ret
    if ret is None:
        return False, f"unsupported-return:{field.type}", arg, None
    if arg.kind == "string" and arg.cxx_type.endswith("*"):
        return False, "string-pointer", arg, ret
    if ret.kind == "object" and ret.class_name in INACCESSIBLE_CLASSES:
        return False, f"inaccessible-type:{ret.class_name}", arg, ret
    if arg.kind == "object" and arg.class_name in INACCESSIBLE_CLASSES:
        return False, f"inaccessible-type:{arg.class_name}", arg, ret
    return True, "", arg, ret
