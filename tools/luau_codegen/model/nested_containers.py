from __future__ import annotations


AUDITED_POINTER_GRID_KIND = "audited_pointer_grid"


# Audited pointer grids for GJBaseGameLayer, only these use the field adapter.
AUDITED_GJ_POINTER_FIELDS: dict[tuple[str, str], tuple[str, str, bool]] = {
    ("GJBaseGameLayer", "m_sectionSizes"): (
        "gd::vector<gd::vector<int>*>",
        "gd::vector<gd::vector<int>>",
        False,
    ),
    ("GJBaseGameLayer", "m_nonEffectObjectsSizes"): (
        "gd::vector<gd::vector<int>*>",
        "gd::vector<gd::vector<int>>",
        False,
    ),
    ("GJBaseGameLayer", "m_collisionBlockSectionSizes"): (
        "gd::vector<gd::vector<int>*>",
        "gd::vector<gd::vector<int>>",
        False,
    ),
    ("GJBaseGameLayer", "m_nonEffectObjectsFlags"): (
        "gd::vector<gd::vector<bool>*>",
        "gd::vector<gd::vector<bool>>",
        True,
    ),
    ("GJBaseGameLayer", "m_collisionBlockSections"): (
        "gd::vector<gd::vector<GameObject*>*>",
        "gd::vector<gd::vector<GameObject*>>",
        True,
    ),
    ("GJBaseGameLayer", "m_sections"): (
        "gd::vector<gd::vector<gd::vector<GameObject*>*>*>",
        "gd::vector<gd::vector<gd::vector<GameObject*>>>",
        True,
    ),
    ("GJBaseGameLayer", "m_nonEffectObjects"): (
        "gd::vector<gd::vector<gd::vector<GameObject*>*>*>",
        "gd::vector<gd::vector<gd::vector<GameObject*>>>",
        True,
    ),
}


def audited_gj_pointer_field_spec(
    owner_class: str,
    field_name: str,
    normalized: str,
) -> tuple[str, bool] | None:
    spec = AUDITED_GJ_POINTER_FIELDS.get((owner_class, field_name))
    if spec is None or spec[0] != normalized:
        return None
    return spec[1], spec[2]
