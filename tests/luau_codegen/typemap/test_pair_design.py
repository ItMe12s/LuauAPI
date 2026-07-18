from __future__ import annotations

import unittest

from luau_codegen.convert.type_map import classify_arg  # type: ignore[import-unresolved]
from luau_codegen.model.pair_design import (  # type: ignore[import-unresolved]
    BASELINE_PAIR_SKIP_FIELDS,
    PAIR_COMPONENT_KINDS,
    PAIR_RECORD_FIELDS,
    PHASE_PAIR_MAP_KEY,
    PHASE_PAIR_MAP_VALUE,
    PHASE_PAIR_VALUE_AND_ELEMENTS,
    pair_key_map_lua_type,
    pair_lua_type,
    pair_map_value_lua_type,
)


class PairDesignPolicyTests(unittest.TestCase):
    def test_record_tuple_fields(self) -> None:
        self.assertEqual(PAIR_RECORD_FIELDS, ("first", "second"))

    def test_pair_lua_type_uses_named_record(self) -> None:
        self.assertEqual(
            pair_lua_type("number", "boolean"),
            "{ first: number, second: boolean }",
        )

    def test_pair_map_value_keeps_dictionary_keys(self) -> None:
        inner = pair_lua_type("number", "number")
        self.assertEqual(
            pair_map_value_lua_type("number", inner),
            "{ [number]: { first: number, second: number } }",
        )

    def test_pair_key_map_uses_entry_list_not_dictionary(self) -> None:
        shape = pair_key_map_lua_type("number")
        self.assertIn("value:", shape)
        self.assertNotIn("[", shape)

    def test_baseline_skip_fields_documented(self) -> None:
        self.assertEqual(len(BASELINE_PAIR_SKIP_FIELDS), 4)
        self.assertIn("m_targetGroups", BASELINE_PAIR_SKIP_FIELDS)
        self.assertIn("std::pair", BASELINE_PAIR_SKIP_FIELDS["m_destroyObjectValues"])

    def test_phases_order_values_before_keys(self) -> None:
        self.assertLess(PHASE_PAIR_VALUE_AND_ELEMENTS, PHASE_PAIR_MAP_VALUE)
        self.assertLess(PHASE_PAIR_MAP_VALUE, PHASE_PAIR_MAP_KEY)

    def test_pair_components_align_with_map_values(self) -> None:
        self.assertIn("object", PAIR_COMPONENT_KINDS)
        self.assertIn("enum", PAIR_COMPONENT_KINDS)
        self.assertNotIn("pair", PAIR_COMPONENT_KINDS)

    def test_std_pair_classifies_as_record(self) -> None:
        info = classify_arg("std::pair<int, int>", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "pair")
        self.assertEqual(info.lua_type, "{ first: number, second: number }")

    def test_baseline_map_with_pair_value_classifies(self) -> None:
        info = classify_arg("gd::unordered_map<int, std::pair<int, int>>", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "unordered_map")
        self.assertIn("first: number", info.lua_type)
        self.assertIn("[number]", info.lua_type)

    def test_baseline_map_with_pair_key_classifies_entry_list(self) -> None:
        info = classify_arg("gd::map<std::pair<int, int>, int>", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "map")
        self.assertIn("value:", info.lua_type)
        self.assertNotIn("[", info.lua_type)

    def test_pair_key_map_with_enum_component_classifies(self) -> None:
        info = classify_arg("gd::map<std::pair<int, UnlockType>, int>", {})
        self.assertIsNotNone(info)
        assert info is not None
        assert info.key_type is not None
        self.assertEqual(info.key_type.kind, "pair")

    def test_vector_of_pair_classifies(self) -> None:
        info = classify_arg("gd::vector<std::pair<int, int>>", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "vector")
        self.assertIn("first: number", info.lua_type)
