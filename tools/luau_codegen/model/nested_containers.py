from __future__ import annotations

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from luau_codegen.convert.type_map import TypeInfo

BASELINE_NESTED_MAP_FIELDS: dict[str, str] = {
    "m_unkMap770": "gd::map<std::pair<int, int>, gd::vector<GroupCommandObject2*>>",
}

BASELINE_NESTED_UNORDERED_MAP_FIELDS: dict[str, str] = {
    "m_labelObjects": "gd::unordered_map<int, gd::vector<LabelGameObject*>>",
    "m_timeLabelObjects": "gd::unordered_map<int, gd::vector<LabelGameObject*>>",
}

BASELINE_NESTED_PRIMITIVE_VECTOR_FIELDS: dict[str, str] = {
    "m_sectionSizes": "gd::vector<gd::vector<int>*>",
    "m_nonEffectObjectsSizes": "gd::vector<gd::vector<int>*>",
    "m_collisionBlockSectionSizes": "gd::vector<gd::vector<int>*>",
}

BASELINE_NESTED_BOOL_VECTOR_FIELDS: dict[str, str] = {
    "m_nonEffectObjectsFlags": "gd::vector<gd::vector<bool>*>",
}

BASELINE_NESTED_OBJECT_VECTOR_FIELDS: dict[str, str] = {
    "m_collisionBlockSections": "gd::vector<gd::vector<GameObject*>*>",
}

BASELINE_NESTED_OBJECT_GRID_FIELDS: dict[str, str] = {
    "m_sections": "gd::vector<gd::vector<gd::vector<GameObject*>*>*>",
    "m_nonEffectObjects": "gd::vector<gd::vector<gd::vector<GameObject*>*>*>",
}

BASELINE_MAP_VECTOR_FIELDS: dict[str, str] = {
    "m_spawnRemapTriggers": "gd::vector<gd::unordered_map<int,int>>",
}

BASELINE_TUPLE_SET_FIELDS: dict[str, str] = {
    "m_spawnTuples": "gd::set<std::tuple<int, int, int>>",
}

ALLOWED_NESTED_PRIMITIVE_VECTOR_INNER = "gd::vector<int>*"
ALLOWED_NESTED_BOOL_VECTOR_INNER = "gd::vector<bool>*"
ALLOWED_NESTED_OBJECT_VECTOR_OUTER = "gd::vector<gd::vector<GameObject*>*>"
ALLOWED_NESTED_OBJECT_GRID_OUTER = "gd::vector<gd::vector<gd::vector<GameObject*>*>*>"
ALLOWED_MAP_VECTOR_ELEMENT = "gd::unordered_map<int,int>"
ALLOWED_INT_TUPLE = "std::tuple<int, int, int>"


def allow_nested_map_value(key: TypeInfo, value: TypeInfo) -> bool:
    element = value.element_type
    if value.kind == "vector_view":
        if element is None or element.kind not in ("object", "opaque_handle"):
            return False
    elif value.kind == "primitive_vector":
        if element is None or element.kind != "value":
            return False
    else:
        return False
    if key.kind == "pair":
        if key.key_type is None or key.value_type is None:
            return False
        return key.key_type.kind in ("number", "enum") and key.value_type.kind in (
            "number",
            "enum",
        )
    return key.kind in ("bool", "number", "wideint", "string", "enum")


def allow_nested_primitive_vector_outer(normalized: str) -> bool:
    return normalized == f"gd::vector<{ALLOWED_NESTED_PRIMITIVE_VECTOR_INNER}>"


def allow_nested_bool_vector_outer(normalized: str) -> bool:
    return normalized == f"gd::vector<{ALLOWED_NESTED_BOOL_VECTOR_INNER}>"


def allow_nested_object_vector_outer(normalized: str) -> bool:
    return normalized == ALLOWED_NESTED_OBJECT_VECTOR_OUTER


def allow_nested_object_grid_outer(normalized: str) -> bool:
    return normalized == ALLOWED_NESTED_OBJECT_GRID_OUTER


def allow_map_vector_element(normalized: str) -> bool:
    return normalized == ALLOWED_MAP_VECTOR_ELEMENT


def allow_int_tuple(normalized: str) -> bool:
    return normalized == ALLOWED_INT_TUPLE
