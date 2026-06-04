from __future__ import annotations

PAIR_RECORD_FIELDS: tuple[str, str] = ("first", "second")

PAIR_COMPONENT_KINDS: frozenset[str] = frozenset(
    {"bool", "number", "wideint", "string", "enum", "value", "object"}
)

BASELINE_PAIR_SKIP_FIELDS: dict[str, str] = {
    "m_targetGroups": "gd::unordered_map<int, std::pair<int, int>>",
    "m_destroyObjectValues": "gd::map<std::pair<int, int>, std::pair<float, float>>",
    "m_unkMap578": "gd::unordered_map<int, std::pair<double, double>>",
    "m_accountIDForIcon": "gd::map<std::pair<int, UnlockType>, int>",
}

PHASE_PAIR_VALUE_AND_ELEMENTS = 1
PHASE_PAIR_MAP_VALUE = 2
PHASE_PAIR_MAP_KEY = 3


def pair_lua_type(first_lua: str, second_lua: str) -> str:
    return f"{{ {PAIR_RECORD_FIELDS[0]}: {first_lua}, {PAIR_RECORD_FIELDS[1]}: {second_lua} }}"


def pair_map_value_lua_type(key_lua: str, value_lua: str) -> str:
    return f"{{ [{key_lua}]: {value_lua} }}"


def pair_key_map_entry_lua_type(first_lua: str, second_lua: str, value_lua: str) -> str:
    return (
        "{ "
        f"{PAIR_RECORD_FIELDS[0]}: {first_lua}, "
        f"{PAIR_RECORD_FIELDS[1]}: {second_lua}, "
        f"value: {value_lua} "
        "}"
    )


def pair_key_map_lua_type(value_lua: str) -> str:
    entry = pair_key_map_entry_lua_type("number", "number", value_lua)
    return f"{{ {entry} }}"
