from __future__ import annotations

import unittest

import test_support
from luau_codegen.convert.type_classification import classify_arg  # type: ignore[import-unresolved]
from luau_codegen.model.value_struct_gate import (  # type: ignore[import-unresolved]
    VALUE_STRUCT_OPT_IN,
)


class FmodSurfaceTests(unittest.TestCase):
    def test_fmod_system_dsp_opaque_registered(self) -> None:
        cases = {
            "FMOD::System*": "FMODSystem",
            "FMOD::DSP*": "FMODDSP",
        }
        for cxx, lua in cases.items():
            info = classify_arg(cxx, {})
            self.assertIsNotNone(info)
            assert info is not None
            self.assertEqual(info.kind, "opaque_handle")
            self.assertEqual(info.lua_type, lua)

    def test_fmod_sound_renamed_to_handle(self) -> None:
        info = classify_arg("FMOD::Sound*", {}, ctx=test_support.fixture_codegen_context())
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "opaque_handle")
        self.assertEqual(info.lua_type, "FMODSoundHandle")
        self.assertNotEqual(info.lua_type, "FMODSound")

    def test_opaque_map_value_allowed(self) -> None:
        info = classify_arg("gd::unordered_map<int, FMOD::Channel*>", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "unordered_map")
        assert info.value_type is not None
        self.assertEqual(info.value_type.kind, "opaque_handle")
        self.assertEqual(info.lua_type, "{ [number]: FMODChannel? }")

    def test_fmod_music_struct_round_trip(self) -> None:
        self.assertIn("FMODMusic", VALUE_STRUCT_OPT_IN)
        ctx = test_support.fixture_codegen_context()
        self.assertIn("FMODMusic", ctx.value_types.types)
        info = classify_arg("FMODMusic", {}, ctx=ctx)
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "value")

    def test_value_struct_pointers_are_not_treated_as_values(self) -> None:
        ctx = test_support.fixture_codegen_context()
        self.assertIsNone(classify_arg("cocos2d::CCPoint*", {}, ctx=ctx))
        self.assertIsNone(classify_arg("PulseEffectAction*", {}, ctx=ctx))

        sound = classify_arg("FMODSound*", {}, ctx=ctx)
        self.assertIsNotNone(sound)
        assert sound is not None
        self.assertEqual(sound.kind, "opaque_handle")
