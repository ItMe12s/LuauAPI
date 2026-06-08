from __future__ import annotations

import unittest

from luau_codegen.parse.cpp_scan import (  # type: ignore[import-unresolved]
    balanced_delimiter_end,
    find_unbalanced,
    template_preceded,
)


class CppScanTests(unittest.TestCase):
    def test_balanced_end_ignores_braces_in_string_literals(self) -> None:
        text = 'void f() { const char* s = "{ not a brace"; if (true) { return; } }'
        start = text.index("{")
        end = balanced_delimiter_end(text, start)
        self.assertEqual(
            text[start : end + 1], '{ const char* s = "{ not a brace"; if (true) { return; } }'
        )

    def test_find_unbalanced_ignores_semicolon_in_string(self) -> None:
        text = 'const char* s = "a; b"; int x'
        semi = find_unbalanced(text, 0, ";")
        self.assertEqual(semi, text.index("; int"))

    def test_template_preceded_handles_nested_angle_brackets(self) -> None:
        header = "template<std::vector<int>> class GEODE_DLL Foo {"
        pos = header.index("class")
        self.assertTrue(template_preceded(header, pos))

    def test_template_preceded_false_without_template(self) -> None:
        header = "class GEODE_DLL Foo {"
        self.assertFalse(template_preceded(header, 0))
