from __future__ import annotations

import unittest

import test_support
from luau_codegen.convert.type_map import (  # type: ignore[import-unresolved]
    VALUE_TYPES,
    classify_arg,
)
from luau_codegen.emit.luau_types.references import (  # type: ignore[import-unresolved]
    _VALUE_STUB_BODY,
    _emit_value_stub_block,
)
from luau_codegen.model.value_struct_gate import (  # type: ignore[import-unresolved]
    VALUE_STRUCT_OPT_IN,
)


class ValueStructGatePolicyTests(unittest.TestCase):
    def test_opt_in_is_nonempty_and_unique(self) -> None:
        self.assertGreater(len(VALUE_STRUCT_OPT_IN), 0)
        self.assertEqual(len(VALUE_STRUCT_OPT_IN), len(set(VALUE_STRUCT_OPT_IN)))

    def test_chance_object_is_opted_in(self) -> None:
        self.assertIn("ChanceObject", VALUE_STRUCT_OPT_IN)

    def test_big_state_blobs_opted_in(self) -> None:
        for name in (
            "GJGameState",
            "GJShaderState",
            "FMODAudioState",
            "GJTransformState",
            "SequenceTriggerState",
            "EffectManagerState",
            "PulseEffectAction",
        ):
            self.assertIn(name, VALUE_STRUCT_OPT_IN)

    def test_smart_prefab_result_still_bound(self) -> None:
        self.assertNotIn("SmartPrefabResult", VALUE_STRUCT_OPT_IN)
        info = classify_arg("SmartPrefabResult", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "value")

    def test_smart_prefab_result_stub_emits_with_gj_smart_prefab_dep(self) -> None:
        block = _emit_value_stub_block({"SmartPrefabResult"})
        self.assertIn("export type SmartPrefabResult", block)
        self.assertIn("GJSmartPrefab?", block)
        self.assertIn("SmartPrefabResult", _VALUE_STUB_BODY)


if __name__ == "__main__":
    unittest.main()
