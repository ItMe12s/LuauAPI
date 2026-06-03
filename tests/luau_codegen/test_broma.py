from __future__ import annotations

import os
import unittest
from helpers import (
    Arg,  # type: ignore[import-unresolved]
    Class,  # type: ignore[import-unresolved]
    Method,  # type: ignore[import-unresolved]
    Root,  # type: ignore[import-unresolved]
    class_link_platforms,  # type: ignore[import-unresolved]
    collect_plan,  # type: ignore[import-unresolved]
    is_link_platform,  # type: ignore[import-unresolved]
    parse_file,  # type: ignore[import-unresolved]
    supported,  # type: ignore[import-unresolved]
)


class AccessLevelTests(unittest.TestCase):
    def test_protected_method_rejected(self) -> None:
        cls = Class(name="CCTextFieldTTF", attributes=["link(win)"])
        method = Method(
            name="insertText",
            ret="void",
            args=[
                Arg("char const*", "text"),
                Arg("int", "len"),
                Arg("cocos2d::enumKeyCodes", "key"),
            ],
            access="protected",
            platforms={"win": "link"},
        )

        ok, reason = supported(cls, method, {"CCTextFieldTTF": cls}, "win")

        self.assertFalse(ok)
        self.assertEqual(reason, "inaccessible")

    def test_parse_protected_prefix(self) -> None:
        import tempfile

        bro = """
class Foo {
    protected virtual void insertText(char const* text, int len);
};
"""
        with tempfile.NamedTemporaryFile("w", suffix=".bro", delete=False) as f:
            f.write(bro)
            path = f.name
        try:
            root = parse_file(path)
            methods = root.classes[0].methods
            self.assertEqual(len(methods), 1)
            self.assertEqual(methods[0].name, "insertText")
            self.assertEqual(methods[0].access, "protected")
        finally:
            os.unlink(path)

    def test_parse_access_section(self) -> None:
        import tempfile

        bro = """
class Foo {
protected:
    virtual void insertText(char const* text);
};
"""
        with tempfile.NamedTemporaryFile("w", suffix=".bro", delete=False) as f:
            f.write(bro)
            path = f.name
        try:
            root = parse_file(path)
            methods = root.classes[0].methods
            self.assertEqual(len(methods), 1)
            self.assertEqual(methods[0].access, "protected")
        finally:
            os.unlink(path)


class CompoundLinkAttributeTests(unittest.TestCase):
    def test_compound_link_attribute_matches_android64(self) -> None:
        cls = Class(
            name="GameManager",
            attributes=["link(android)", "depends(UIButtonConfig)"],
        )

        self.assertTrue(is_link_platform(cls, "android64"))
        self.assertFalse(is_link_platform(cls, "win"))

    def test_compound_link_attribute_string_matches_android64(self) -> None:
        cls = Class(
            name="GameManager",
            attributes=["link(android), depends(UIButtonConfig)"],
        )

        self.assertEqual(class_link_platforms(cls), {"android"})
        self.assertTrue(is_link_platform(cls, "android64"))

    def test_linked_game_manager_method_supported_on_android64(self) -> None:
        cls = Class(
            name="GameManager",
            attributes=["link(android)", "depends(UIButtonConfig)"],
        )
        method = Method(
            name="update",
            ret="void",
            args=[Arg("float", "dt")],
            platforms={"win": "0x189bc0", "imac": "0x38bb20"},
        )

        ok, reason = supported(cls, method, {"GameManager": cls}, "android64")

        self.assertTrue(ok)
        self.assertEqual(reason, "")


class BareInlineTests(unittest.TestCase):
    def test_bare_inline_callable_on_win(self) -> None:
        cls = Class(name="GameManager")
        method = Method(
            name="getPlayLayer",
            ret="PlayLayer*",
            args=[],
            platforms={"inline": "inline"},
        )

        ok, reason = supported(
            cls,
            method,
            {"GameManager": cls, "PlayLayer": Class(name="PlayLayer")},
            "win",
        )

        self.assertTrue(ok)
        self.assertEqual(reason, "")

    def test_platform_inline_callable_on_ios(self) -> None:
        cls = Class(name="SimplePlayer", bases=["cocos2d::CCSprite"])
        method = Method(
            name="setColors",
            ret="void",
            args=[
                Arg("cocos2d::ccColor3B const&", "color1"),
                Arg("cocos2d::ccColor3B const&", "color2"),
            ],
            platforms={
                "win": "inline",
                "imac": "0x36edb0",
                "m1": "0x2f8c50",
                "ios": "inline",
            },
        )
        objects = {
            "SimplePlayer": cls,
            "cocos2d::CCSprite": Class(name="CCSprite", namespace="cocos2d"),
        }

        ok, reason = supported(cls, method, objects, "ios")

        self.assertTrue(ok)
        self.assertEqual(reason, "")

    def test_platform_inline_survives_intersection(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="SimplePlayer",
            bases=["cocos2d::CCSprite"],
            methods=[
                Method(
                    name="setColors",
                    ret="void",
                    args=[
                        Arg("cocos2d::ccColor3B const&", "color1"),
                        Arg("cocos2d::ccColor3B const&", "color2"),
                    ],
                    platforms={
                        "win": "inline",
                        "imac": "0x36edb0",
                        "m1": "0x2f8c50",
                        "ios": "inline",
                        "android32": "0x1",
                        "android64": "0x1",
                    },
                )
            ],
        )
        root = Root(
            classes=[
                ccobject,
                Class(name="CCSprite", namespace="cocos2d", bases=["CCObject"]),
                cls,
            ]
        )

        plan = collect_plan(root, "win")

        self.assertIn("setColors", plan.supported_by_class["SimplePlayer"])


class PlatformFieldBlockTests(unittest.TestCase):
    def test_android_ios_block_parses_inner_fields(self) -> None:
        import tempfile

        bro = """
class Foo {
    float m_offset;
    android, ios {
        int m_spawnCount;
    }
};
"""
        with tempfile.NamedTemporaryFile("w", suffix=".bro", delete=False) as f:
            f.write(bro)
            path = f.name
        try:
            root = parse_file(path)
            fields = root.classes[0].fields
            self.assertEqual(
                [field.name for field in fields], ["m_offset", "m_spawnCount"]
            )
            self.assertIsNone(fields[0].platforms)
            self.assertEqual(
                fields[1].platforms,
                frozenset({"android32", "android64", "ios"}),
            )
            self.assertFalse(any(field.name == "ios" for field in fields))
        finally:
            os.unlink(path)

    def test_platform_block_header_not_a_field(self) -> None:
        from luau_codegen.parse.broma import _parse_platform_block_header  # type: ignore[import-unresolved]

        self.assertIsNone(_parse_platform_block_header("void foo()"))
        self.assertEqual(
            _parse_platform_block_header("android, ios"),
            frozenset({"android32", "android64", "ios"}),
        )


class F3MethodAttrSplitTests(unittest.TestCase):
    def test_compound_method_attrs_split(self) -> None:
        from luau_codegen.parse.broma import Cursor as _Cursor, parse_class_body as _parse_class_body  # type: ignore[import-unresolved]

        cls = Class(name="Foo", namespace="test")
        bro = "[[link(win), missing(android)]] void bar() = win 0x1234; }"
        cursor = _Cursor(bro)
        _parse_class_body(cursor, cls)
        self.assertTrue(len(cls.methods) > 0)
        m = cls.methods[0]
        self.assertIn("link(win)", m.attributes)
        self.assertIn("missing(android)", m.attributes)
        self.assertEqual(len(m.attributes), 2)
