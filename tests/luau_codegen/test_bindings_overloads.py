from __future__ import annotations

import os
import shutil
import tempfile
import unittest
from helpers import (
    Arg,  # type: ignore[import-unresolved]
    Class,  # type: ignore[import-unresolved]
    Method,  # type: ignore[import-unresolved]
    android_symbol,  # type: ignore[import-unresolved]
    codegen_main,  # type: ignore[import-unresolved]
    group_supported,  # type: ignore[import-unresolved]
)


class F5OverloadFirstDeclaredWinsTests(unittest.TestCase):
    def test_first_overload_kept_on_arity_collision(self) -> None:
        cls = Class(
            name="TestObj",
            namespace="cocos2d",
            bases=["CCObject"],
            attributes=["link(win)"],
        )
        m1 = Method(
            name="foo",
            ret="void",
            args=[Arg(type="int", name="a")],
            platforms={"win": "0x1"},
        )
        m2 = Method(
            name="foo",
            ret="void",
            args=[Arg(type="float", name="b")],
            platforms={"win": "0x2"},
        )
        cls.methods = [m1, m2]
        objects = {"TestObj": cls, "cocos2d::TestObj": cls}
        grouped, skipped = group_supported(cls, objects, "win")
        self.assertIn("foo", grouped)
        self.assertEqual(len(grouped["foo"]), 1)
        self.assertIs(grouped["foo"][0], m1)
        self.assertEqual(len(skipped), 1)
        self.assertIn("ambiguous-overload-arity", skipped[0][1])

    def test_fail_on_ambiguous_overload_flag_exits_nonzero(self) -> None:
        tmpdir = tempfile.mkdtemp()
        try:
            with open(
                os.path.join(tmpdir, "GeometryDash.bro"), "w", encoding="utf-8"
            ) as f:
                f.write(
                    "class cocos2d::CCObject {};"
                    "class gd::TestObj : cocos2d::CCObject {"
                    "void foo(int a) = win 0x1;"
                    "void foo(float b) = win 0x2;"
                    "};"
                )
            code = codegen_main(
                [
                    "--bindings",
                    tmpdir,
                    "--platform",
                    "win",
                    "--list-outputs",
                    "--fail-on-ambiguous-overload",
                ]
            )
            self.assertEqual(code, 6)
        finally:
            shutil.rmtree(tmpdir)

    def test_ambiguous_overload_default_does_not_fail(self) -> None:
        tmpdir = tempfile.mkdtemp()
        try:
            with open(
                os.path.join(tmpdir, "GeometryDash.bro"), "w", encoding="utf-8"
            ) as f:
                f.write(
                    "class cocos2d::CCObject {};"
                    "class gd::TestObj : cocos2d::CCObject {"
                    "void foo(int a) = win 0x1;"
                    "void foo(float b) = win 0x2;"
                    "};"
                )
            code = codegen_main(
                ["--bindings", tmpdir, "--platform", "win", "--list-outputs"]
            )
            self.assertEqual(code, 0)
        finally:
            shutil.rmtree(tmpdir)

    def test_preferred_overload_selected_over_first_declared(self) -> None:
        cls = Class(
            name="CCScale9Sprite",
            namespace="cocos2d",
            bases=["CCObject"],
            attributes=["link(win)"],
        )
        loser = Method(
            name="create",
            ret="bool",
            args=[
                Arg(type="cocos2d::CCRect", name="rect"),
                Arg(type="char const*", name="file"),
            ],
            is_static=True,
            platforms={"win": "0x1"},
        )
        winner = Method(
            name="create",
            ret="bool",
            args=[
                Arg(type="char const*", name="file"),
                Arg(type="cocos2d::CCRect", name="rect"),
            ],
            is_static=True,
            platforms={"win": "0x2"},
        )
        cls.methods = [loser, winner]
        objects = {"CCScale9Sprite": cls, "cocos2d::CCScale9Sprite": cls}
        grouped, skipped = group_supported(cls, objects, "win")
        self.assertEqual(len(grouped["create"]), 1)
        self.assertIs(grouped["create"][0], winner)
        reasons = [reason for _, reason in skipped]
        self.assertIn("overload-superseded:2", reasons)
        self.assertFalse(
            any(r.startswith("ambiguous-overload-arity") for r in reasons),
            f"resolved overload should not be flagged ambiguous: {reasons}",
        )

    def test_border_set_padding_prefers_scalar_over_point(self) -> None:
        cls = Class(
            name="Border",
            namespace="geode",
            bases=["CCObject"],
            attributes=["link(win)"],
        )
        point = Method(
            name="setPadding",
            ret="void",
            args=[Arg(type="cocos2d::CCPoint const&", name="padding")],
            platforms={"win": "0x1"},
        )
        xy = Method(
            name="setPadding",
            ret="void",
            args=[Arg(type="float", name="x"), Arg(type="float", name="y")],
            platforms={"win": "0x2"},
        )
        scalar = Method(
            name="setPadding",
            ret="void",
            args=[Arg(type="float", name="padding")],
            platforms={"win": "0x3"},
        )
        cls.methods = [point, xy, scalar]
        objects = {"Border": cls, "geode::Border": cls}
        grouped, skipped = group_supported(cls, objects, "win")
        kept = grouped["setPadding"]
        self.assertEqual(len(kept), 2)
        kept_by_arity = {len(m.args): m for m in kept}
        self.assertIs(kept_by_arity[1], scalar)
        self.assertIs(kept_by_arity[2], xy)
        reasons = [reason for _, reason in skipped]
        self.assertIn("overload-superseded:1", reasons)
        self.assertFalse(
            any(r.startswith("ambiguous-overload-arity") for r in reasons),
            f"resolved overload should not be flagged ambiguous: {reasons}",
        )

    def test_gj_item_icon_darken_store_item_prefers_shop_type(self) -> None:
        cls = Class(
            name="GJItemIcon",
            namespace="gd",
            bases=["CCSprite"],
            attributes=["link(win)"],
        )
        shop_type = Method(
            name="darkenStoreItem",
            ret="void",
            args=[Arg(type="ShopType", name="type")],
            platforms={"win": "0x1"},
        )
        color = Method(
            name="darkenStoreItem",
            ret="void",
            args=[Arg(type="cocos2d::ccColor3B", name="color")],
            platforms={"win": "0x2"},
        )
        cls.methods = [shop_type, color]
        objects = {"GJItemIcon": cls, "gd::GJItemIcon": cls, "CCSprite": cls}
        grouped, skipped = group_supported(cls, objects, "win")
        self.assertEqual(len(grouped["darkenStoreItem"]), 1)
        self.assertIs(grouped["darkenStoreItem"][0], shop_type)
        reasons = [reason for _, reason in skipped]
        self.assertIn("overload-superseded:1", reasons)
        self.assertFalse(
            any(r.startswith("ambiguous-overload-arity") for r in reasons),
            f"resolved overload should not be flagged ambiguous: {reasons}",
        )

    def test_unmatched_preference_falls_back_to_ambiguous(self) -> None:
        cls = Class(
            name="CCScale9Sprite",
            namespace="cocos2d",
            bases=["CCObject"],
            attributes=["link(win)"],
        )
        first = Method(
            name="create",
            ret="bool",
            args=[Arg(type="int", name="a"), Arg(type="int", name="b")],
            is_static=True,
            platforms={"win": "0x1"},
        )
        second = Method(
            name="create",
            ret="bool",
            args=[Arg(type="float", name="a"), Arg(type="float", name="b")],
            is_static=True,
            platforms={"win": "0x2"},
        )
        cls.methods = [first, second]
        objects = {"CCScale9Sprite": cls, "cocos2d::CCScale9Sprite": cls}
        grouped, skipped = group_supported(cls, objects, "win")
        self.assertEqual(len(grouped["create"]), 1)
        self.assertIs(grouped["create"][0], first)
        reasons = [reason for _, reason in skipped]
        self.assertIn("ambiguous-overload-arity:2", reasons)


class F8ConstMethodManglingTests(unittest.TestCase):
    def test_const_method_has_K_qualifier(self) -> None:
        cls = Class(name="CCObject", namespace="cocos2d")
        m = Method(
            name="getTag",
            ret="int",
            args=[],
            is_const=True,
            platforms={"android64": "0x1"},
        )
        sym = android_symbol(cls, m)
        self.assertTrue(sym.startswith("_ZNK"), f"Expected _ZNK prefix, got {sym}")
        self.assertIn("6getTag", sym)

    def test_non_const_method_no_K_qualifier(self) -> None:
        cls = Class(name="CCObject", namespace="cocos2d")
        m = Method(
            name="setTag",
            ret="void",
            args=[Arg(type="int", name="t")],
            platforms={"android64": "0x1"},
        )
        sym = android_symbol(cls, m)
        self.assertTrue(sym.startswith("_ZN"), f"Expected _ZN prefix, got {sym}")
        self.assertFalse(sym.startswith("_ZNK"))

    def test_const_vs_non_const_distinct(self) -> None:
        cls = Class(name="CCObject", namespace="cocos2d")
        m_const = Method(
            name="getTag",
            ret="int",
            args=[],
            is_const=True,
            platforms={"android64": "0x1"},
        )
        m_non = Method(
            name="getTag",
            ret="int",
            args=[],
            is_const=False,
            platforms={"android64": "0x1"},
        )
        self.assertNotEqual(android_symbol(cls, m_const), android_symbol(cls, m_non))

    def test_reference_and_const_type_mangling(self) -> None:
        cls = Class(name="Foo")
        method = Method(name="take", ret="void", args=[Arg("int const&", "value")])
        self.assertEqual(android_symbol(cls, method), "_ZN3Foo4takeERKi")

    def test_repeated_pointer_uses_substitution(self) -> None:
        cls = Class(name="Bar")
        method = Method(
            name="take",
            ret="void",
            args=[Arg("Foo*", "a"), Arg("Foo*", "b")],
        )
        self.assertEqual(android_symbol(cls, method), "_ZN3Bar4takeEP3FooS0_")

    def test_template_type_mangling_includes_defaults(self) -> None:
        cls = Class(name="Foo")
        method = Method(
            name="take",
            ret="void",
            args=[Arg("gd::vector<int>", "values")],
        )
        self.assertEqual(
            android_symbol(cls, method), "_ZN3Foo4takeEN2gd6vectorIiSaIiEEE"
        )
