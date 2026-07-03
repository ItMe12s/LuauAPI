from __future__ import annotations

import unittest

from test_support import all_platforms
from luau_codegen.emit.bindings.class_file import _emit_class_file  # type: ignore[import-unresolved]
from luau_codegen.emit.luau_types.classes import _emit_class  # type: ignore[import-unresolved]
from luau_codegen.emit.luau_types.method_types import lua_export_name  # type: ignore[import-unresolved]
from luau_codegen.parse.broma import Class, Method  # type: ignore[import-unresolved]


def _fixture(*method_names: str) -> tuple[Class, dict[str, list[Method]], dict[str, Class]]:
    ccobject = Class(name="CCObject", namespace="cocos2d")
    cls = Class(name="KeywordFixture", namespace="cocos2d", bases=["CCObject"])
    methods = [
        Method(name=name, ret="void", args=[], platforms=all_platforms("0x1"))
        for name in method_names
    ]
    cls.methods = methods
    grouped = {name: [method] for name, method in zip(method_names, methods, strict=True)}
    objects = {"CCObject": ccobject, "KeywordFixture": cls}
    return cls, grouped, objects


class KeywordMethodNameTests(unittest.TestCase):
    def test_lua_export_name_renames_keywords(self) -> None:
        self.assertEqual(lua_export_name("visit"), "visit")
        self.assertEqual(lua_export_name("end"), "endToLua")

    def test_lua_export_name_skips_when_native_tolua_exists(self) -> None:
        grouped = {
            "end": [Method(name="end", ret="void", args=[], platforms=all_platforms("0x1"))],
            "endToLua": [
                Method(name="endToLua", ret="void", args=[], platforms=all_platforms("0x2"))
            ],
        }
        self.assertIsNone(lua_export_name("end", grouped))

    def test_stub_emits_endToLua(self) -> None:
        cls, grouped, objects = _fixture("end")
        text = "".join(_emit_class(cls, grouped, [], objects, set(), "win"))
        self.assertIn("function endToLua(self)", text)
        self.assertNotIn("function end(self)", text)

    def test_binding_registers_endToLua(self) -> None:
        cls, grouped, objects = _fixture("end")
        cpp = _emit_class_file(cls, grouped, [], [], objects, set(), 1, "win")
        self.assertIn('method(L, "endToLua"', cpp)
        self.assertNotIn('method(L, "end"', cpp)
