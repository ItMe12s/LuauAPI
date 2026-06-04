from __future__ import annotations

import os
import shutil
import tempfile
import unittest
from helpers import (
    Arg,  # type: ignore[import-unresolved]
    Class,  # type: ignore[import-unresolved]
    Method,  # type: ignore[import-unresolved]
    Root,  # type: ignore[import-unresolved]
    collect_bindings_root,  # type: ignore[import-unresolved]
    collect_parity,  # type: ignore[import-unresolved]
    collect_plan,  # type: ignore[import-unresolved]
    collect_platform_plan,  # type: ignore[import-unresolved]
    emit_luau_bindings,  # type: ignore[import-unresolved]
    emit_luau_types,  # type: ignore[import-unresolved]
    hookable,  # type: ignore[import-unresolved]
    intersection_platforms,  # type: ignore[import-unresolved]
    parse_file,  # type: ignore[import-unresolved]
    plan_outputs,  # type: ignore[import-unresolved]
    types_text,  # type: ignore[import-unresolved]
)


class PlatformFieldPlanTests(unittest.TestCase):
    def test_platform_scoped_field_targets_follow_platform_plan(self) -> None:
        bro = """
[[link(win, android)]]
class Foo : CCObject {
    void touch() = win 0x1, ios 0x2, m1 0x3, android32 0x4, android64 0x5;
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
            ccobject = Class(name="CCObject", namespace="cocos2d")
            root.classes.insert(0, ccobject)
            win_plan = collect_platform_plan(root, "win")
            ios_plan = collect_platform_plan(root, "ios")
            win_names = [field.name for _, field in win_plan.field_targets_by_class["Foo"]]
            ios_names = [field.name for _, field in ios_plan.field_targets_by_class["Foo"]]
            self.assertEqual(win_names, ["m_offset"])
            self.assertEqual(ios_names, ["m_offset", "m_spawnCount"])
        finally:
            os.unlink(path)


class PlanRegressionTests(unittest.TestCase):
    def test_scanned_geode_ui_class_survives_intersection(self) -> None:
        from luau_codegen.parse.geode_sdk import _SCANNED_LINK_ATTR  # type: ignore[import-unresolved]

        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccnode = Class(name="CCNode", namespace="cocos2d", bases=["CCObject"])
        overlay = Class(
            name="OverlayManager",
            namespace="geode",
            bases=["cocos2d::CCNode"],
            attributes=[_SCANNED_LINK_ATTR],
            methods=[
                Method(
                    name="get",
                    ret="OverlayManager*",
                    args=[],
                    is_static=True,
                )
            ],
        )
        root = Root(classes=[ccobject, ccnode, overlay])

        plan = collect_plan(root, "win")
        self.assertNotIn("OverlayManager", plan.skipped_classes)

        files = emit_luau_types(root, "win", plan=plan)
        self.assertIn("OverlayManagerFactory", types_text(files))

    def test_intersection_platforms_uses_imac_for_intel_mac(self) -> None:
        self.assertEqual(
            intersection_platforms("imac"),
            ("win", "imac", "ios", "android32", "android64"),
        )
        self.assertEqual(
            intersection_platforms("m1"),
            ("win", "m1", "ios", "android32", "android64"),
        )

    def test_collect_parity_uses_target_mac_axis(self) -> None:
        root = Root(classes=[Class(name="CCObject", namespace="cocos2d")])
        imac_data = collect_parity(root, target_platform="imac")
        m1_data = collect_parity(root, target_platform="m1")
        self.assertIn("imac", imac_data["platforms"])
        self.assertNotIn("m1", imac_data["platforms"])
        self.assertIn("imac", imac_data["intersection"]["platforms"])
        self.assertNotIn("m1", imac_data["intersection"]["platforms"])
        self.assertIn("m1", m1_data["platforms"])
        self.assertNotIn("imac", m1_data["platforms"])
        self.assertIn("m1", m1_data["intersection"]["platforms"])
        self.assertNotIn("imac", m1_data["intersection"]["platforms"])
        self.assertEqual(imac_data["hints"]["macPlatform"], "imac")
        self.assertEqual(m1_data["hints"]["macPlatform"], "m1")
        self.assertIn("macGapReasons", imac_data["hints"])
        self.assertNotIn("m1GapReasons", imac_data["hints"])

    def test_imac_intersection_keeps_methods_without_m1_offset(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="IntelOnly",
            namespace="gd",
            bases=["cocos2d::CCObject"],
            attributes=["link(win)"],
        )
        cls.methods = [
            Method(
                name="intelMethod",
                ret="void",
                args=[],
                platforms={
                    "win": "0x1",
                    "imac": "0x2",
                    "ios": "0x3",
                    "android32": "0x4",
                    "android64": "0x5",
                },
            )
        ]
        root = Root(classes=[ccobject, cls])
        plan = collect_plan(root, "imac")
        grouped = plan.supported_by_class.get("IntelOnly", {})
        self.assertIn("intelMethod", grouped)

    def test_plan_outputs_matches_binding_emit(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="SampleNode",
            namespace="cocos2d",
            bases=["CCObject"],
            attributes=["link(win)"],
        )
        cls.methods = [Method(name="doThing", ret="void", args=[], platforms={"win": "0x10"})]
        root = Root(classes=[ccobject, cls])
        plan = collect_plan(root, "win")
        listed = set(plan_outputs(root, "win", plan=plan))
        emitted = set(emit_luau_bindings(root, "win", plan=plan)[0].keys())
        self.assertEqual(listed, emitted)

    def test_hookable_allows_const_ref_args(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="GameObject",
            namespace="gd",
            bases=["cocos2d::CCObject"],
            attributes=["link(win)"],
        )
        method = Method(
            name="updateMainColor",
            ret="void",
            args=[Arg("cocos2d::ccColor3B const&", "color")],
            platforms={"win": "0x100"},
        )
        cls.methods = [method]
        objects = {"CCObject": ccobject, "GameObject": cls, "gd::GameObject": cls}
        self.assertTrue(hookable(cls, method, objects, "win"))

    def test_hookable_rejects_out_ref_args(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="GameManager",
            namespace="gd",
            bases=["cocos2d::CCObject"],
            attributes=["link(win)"],
        )
        method = Method(
            name="getUnlockForAchievement",
            ret="void",
            args=[
                Arg("char const*", "achievement"),
                Arg("UnlockType&", "type"),
            ],
            platforms={"win": "0x200"},
        )
        cls.methods = [method]
        objects = {
            "CCObject": ccobject,
            "GameManager": cls,
            "UnlockType": Class(name="UnlockType"),
        }
        self.assertFalse(hookable(cls, method, objects, "win"))

    def test_duplicate_class_merges_methods(self) -> None:
        tmpdir = tempfile.mkdtemp()
        try:
            bindings = os.path.join(tmpdir, "bindings")
            os.makedirs(bindings)
            bro_a = os.path.join(bindings, "Cocos2d.bro")
            bro_b = os.path.join(bindings, "Extras.bro")
            with open(bro_a, "w", encoding="utf-8") as f:
                f.write(
                    "class cocos2d::MergeTest : cocos2d::CCObject {\n"
                    "    void first() = win 0x1;\n"
                    "}\n"
                )
            with open(bro_b, "w", encoding="utf-8") as f:
                f.write(
                    "class cocos2d::MergeTest : cocos2d::CCObject {\n"
                    "    void second() = win 0x2;\n"
                    "}\n"
                )
            for name in ("FMOD.bro", "Kazmath.bro", "GeometryDash.bro"):
                open(os.path.join(bindings, name), "w", encoding="utf-8").close()
            root = collect_bindings_root(bindings)
            merged = next(c for c in root.classes if c.name == "MergeTest")
            names = {m.name for m in merged.methods}
            self.assertEqual(names, {"first", "second"})
        finally:
            shutil.rmtree(tmpdir)
