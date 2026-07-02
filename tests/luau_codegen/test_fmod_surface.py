from __future__ import annotations

import unittest

import test_support
from luau_codegen.convert.type_map import (  # type: ignore[import-unresolved]
    VALUE_TYPES,
    classify_arg,
)
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
        info = classify_arg("FMOD::Sound*", {})
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
        self.assertIn("FMODMusic", VALUE_TYPES)
        info = classify_arg("FMODMusic", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "value")


if __name__ == "__main__":
    unittest.main()
