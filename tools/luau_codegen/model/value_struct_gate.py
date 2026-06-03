from __future__ import annotations

DENIED_VALUE_STRUCTS: dict[str, str] = {
    "ChanceObject": "no member layout in Broma Extras.bro",
}

GATED_VALUE_STRUCTS: dict[str, str] = {
    "SmartPrefabResult": "SmartPrefabResult",
}

GATED_VALUE_CHECK_CXX: dict[str, str] = {
    lua: cxx for lua, cxx in GATED_VALUE_STRUCTS.items()
}
