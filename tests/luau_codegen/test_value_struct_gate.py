from __future__ import annotations

import unittest

from luau_codegen.convert.type_map import (  # type: ignore[import-unresolved]
    VALUE_TYPES,
    classify_arg,
)
from luau_codegen.emit.luau_types.references import (  # type: ignore[import-unresolved]
    _VALUE_STUB_BODY,
    _emit_value_stub_block,
)
from luau_codegen.model.value_struct_gate import (  # type: ignore[import-unresolved]
    DENIED_VALUE_STRUCTS,
    GATED_VALUE_STRUCTS,
)


class ValueStructGatePolicyTests(unittest.TestCase):
    def test_chance_object_denied_with_reason(self) -> None:
        self.assertIn("ChanceObject", DENIED_VALUE_STRUCTS)
        self.assertNotIn("ChanceObject", GATED_VALUE_STRUCTS)
        self.assertNotIn("ChanceObject", VALUE_TYPES)

    def test_smart_prefab_result_gated_and_classifies(self) -> None:
        self.assertIn("SmartPrefabResult", GATED_VALUE_STRUCTS)
        info = classify_arg("SmartPrefabResult", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "value")

    def test_smart_prefab_result_stub_emits_with_gj_smart_prefab_dep(self) -> None:
        block = _emit_value_stub_block({"SmartPrefabResult"})
        self.assertIn("export type SmartPrefabResult", block)
        self.assertIn("GJSmartPrefab?", block)
        self.assertIn("SmartPrefabResult", _VALUE_STUB_BODY)
