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

ALLOWED_NESTED_PRIMITIVE_VECTOR_INNER = "gd::vector<int>*"


def allow_nested_map_value(key: TypeInfo, value: TypeInfo) -> bool:
    """Map/unordered_map values that are object or opaque vector views only."""
    if value.kind != "vector_view":
        return False
    element = value.element_type
    if element is None or element.kind not in ("object", "opaque_handle"):
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
