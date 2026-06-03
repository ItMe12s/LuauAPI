from __future__ import annotations

import re
from typing import Dict, Set

from luau_codegen.parse.broma import Class, Root
from luau_codegen.model.domain import build_class_lookup, short_name
from luau_codegen.convert.type_map import normalize_type, OPAQUE_HANDLE_TYPES


_VECTOR_PTR_RE = re.compile(r"^gd::vector<(.+)>$")


def _is_ccobject_derived(cls: Class | None, lookup: Dict[str, Class]) -> bool:
    if cls is None:
        return False
    if cls.name == "CCObject" or cls.qualified_name == "cocos2d::CCObject":
        return True
    for base in cls.bases:
        base_cls = lookup.get(short_name(base))
        if _is_ccobject_derived(base_cls, lookup):
            return True
    return False


def vector_pointer_element_types(root: Root) -> Set[str]:
    out: Set[str] = set()
    for cls in root.classes:
        for field in cls.fields:
            normalized = normalize_type(field.type)
            match = _VECTOR_PTR_RE.match(normalized)
            if not match:
                continue
            inner = normalize_type(match.group(1))
            if not inner.endswith("*"):
                continue
            if "vector<" in inner or "map<" in inner or "unordered" in inner:
                continue
            out.add(inner)
    return out


def undocumented_opaque_vector_elements(root: Root) -> Set[str]:
    lookup = build_class_lookup(root.classes)
    undocumented: Set[str] = set()
    for elem in vector_pointer_element_types(root):
        if elem == "void*":
            continue
        short = short_name(elem[:-1])
        if _is_ccobject_derived(lookup.get(short), lookup):
            continue
        if elem not in OPAQUE_HANDLE_TYPES:
            undocumented.add(elem)
    return undocumented
