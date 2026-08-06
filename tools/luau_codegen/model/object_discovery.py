from __future__ import annotations

import re

from luau_codegen.parse.broma import Root
from luau_codegen.model.domain import ClassHierarchy, short_name
from luau_codegen.convert.type_primitives import normalize_type, OPAQUE_HANDLE_TYPES


_VECTOR_PTR_RE = re.compile(r"^gd::vector<(.+)>$")


def vector_pointer_element_types(root: Root) -> set[str]:
    out: set[str] = set()
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


def undocumented_opaque_vector_elements(root: Root) -> set[str]:
    hierarchy = ClassHierarchy(root.classes)
    undocumented: set[str] = set()
    for elem in vector_pointer_element_types(root):
        if elem == "void*":
            continue
        class_name = elem[:-1]
        cls = hierarchy.lookup.get(class_name) or hierarchy.lookup.get(short_name(class_name))
        if hierarchy.is_ccobject_descendant(cls):
            continue
        if elem not in OPAQUE_HANDLE_TYPES:
            undocumented.add(elem)
    return undocumented
