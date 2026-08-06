from __future__ import annotations

import unittest

from test_support import fixture_codegen_context
from luau_codegen.convert.type_classification import classify_arg  # type: ignore[import-unresolved]
from luau_codegen.emit.luau_types.references import (  # type: ignore[import-unresolved]
    _emit_value_stub_block,
)
from luau_codegen.model.value_struct_gate import (  # type: ignore[import-unresolved]
    VALUE_STRUCT_OPT_IN,
)


class ValueStructGatePolicyTests(unittest.TestCase):
    ctx = fixture_codegen_context()

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
        info = classify_arg("SmartPrefabResult", {}, ctx=self.ctx)
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "value")

    def test_smart_prefab_result_stub_emits_with_gj_smart_prefab_dep(self) -> None:
        block = _emit_value_stub_block({"SmartPrefabResult"}, self.ctx.value_types)
        self.assertIn("export type SmartPrefabResult", block)
        self.assertIn("GJSmartPrefab?", block)
        self.assertIn("SmartPrefabResult", self.ctx.value_types.stub_body)


def _assert_opt_in_value_structs(test_case: unittest.TestCase, names: tuple[str, ...]) -> None:
    ctx = fixture_codegen_context()
    missing = [name for name in names if name not in ctx.value_types.types]
    if missing:
        test_case.skipTest("value-struct specs unavailable (bindings dir not built)")
    for name in names:
        test_case.assertIn(name, VALUE_STRUCT_OPT_IN, f"{name} missing from VALUE_STRUCT_OPT_IN")
        test_case.assertIn(name, ctx.value_types.types)
        info = classify_arg(name, {}, ctx=ctx)
        test_case.assertIsNotNone(info, f"{name} classified as None")
        assert info is not None
        test_case.assertEqual(info.kind, "value", f"{name} classified as {info.kind}, not value")


class EffectReplayEditorStructOptInTests(unittest.TestCase):
    STRUCTS = (
        "EnterEffectAnimValue",
        "EnterEffectInstance",
        "SongTriggerState",
        "DynamicObjectAction",
        "GameObjectEditorState",
        "RecordCheckpoint",
        "RecordButtonCommand",
        "PlayerButtonCommand",
    )

    def test_structs_are_opted_in_and_classify_as_value(self) -> None:
        _assert_opt_in_value_structs(self, self.STRUCTS)


class FmodEngineStructOptInTests(unittest.TestCase):
    STRUCTS = (
        "FMODMusic",
        "FMODSound",
        "FMODQueuedEffect",
    )

    def test_structs_are_opted_in_and_classify_as_value(self) -> None:
        _assert_opt_in_value_structs(self, self.STRUCTS)
