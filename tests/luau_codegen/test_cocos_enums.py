from __future__ import annotations

import unittest

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


if __name__ == "__main__":
    unittest.main()
