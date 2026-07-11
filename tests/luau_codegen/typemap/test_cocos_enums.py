from __future__ import annotations

import unittest

from luau_codegen.emit.bindings.cocos_enums import emit_enum_key_codes_manifest  # type: ignore[import-unresolved]
from luau_codegen.model.codegen_context import CodegenContext  # type: ignore[import-unresolved]
from luau_codegen.parse.cocos_enums import parse_enum_key_codes  # type: ignore[import-unresolved]


class CocosEnumTests(unittest.TestCase):
    def test_parse_enum_key_codes_handles_explicit_and_implicit_values(self) -> None:
        entries = parse_enum_key_codes(
            """
            typedef enum {
                KEY_Unknown = -0x01,
                KEY_None = 0x00,
                KEY_A = 0x41,
                KEY_B,
                MOUSE_4 = 0x1100
            } enumKeyCodes;
            """
        )

        self.assertEqual(
            [(entry.name, entry.value) for entry in entries],
            [
                ("KEY_Unknown", -1),
                ("KEY_None", 0),
                ("KEY_A", 0x41),
                ("KEY_B", 0x42),
                ("MOUSE_4", 0x1100),
            ],
        )

    def test_geode_580_controller_keys_reach_cxx_manifest(self) -> None:
        names = (
            "CONTROLLER2_A",
            "CONTROLLER2_B",
            "CONTROLLER2_Y",
            "CONTROLLER2_X",
            "CONTROLLER2_Start",
            "CONTROLLER2_Back",
            "CONTROLLER2_RB",
            "CONTROLLER2_LB",
            "CONTROLLER2_RT",
            "CONTROLLER2_LT",
            "CONTROLLER2_Up",
            "CONTROLLER2_Down",
            "CONTROLLER2_Left",
            "CONTROLLER2_Right",
            "CONTROLLER2_LTHUMBSTICK_UP",
            "CONTROLLER2_LTHUMBSTICK_DOWN",
            "CONTROLLER2_LTHUMBSTICK_LEFT",
            "CONTROLLER2_LTHUMBSTICK_RIGHT",
            "CONTROLLER2_RTHUMBSTICK_UP",
            "CONTROLLER2_RTHUMBSTICK_DOWN",
            "CONTROLLER2_RTHUMBSTICK_LEFT",
            "CONTROLLER2_RTHUMBSTICK_RIGHT",
            "CONTROLLER_L3",
            "CONTROLLER2_L3",
            "CONTROLLER_R3",
            "CONTROLLER2_R3",
        )
        entries = [(name, 0x3EA + index) for index, name in enumerate(names)]
        ctx = CodegenContext.with_geode_enums({}, cocos_enum_members={"enumKeyCodes": entries})

        text = emit_enum_key_codes_manifest(ctx)

        for name in names:
            with self.subTest(name=name):
                self.assertIn(f"X({name})", text)


if __name__ == "__main__":
    unittest.main()
