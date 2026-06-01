from __future__ import annotations

from conftest import *


class AuditReportTests(unittest.TestCase):
    def test_audit_classifies_callback_method(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        target = Class(
            name="Foo",
            bases=["CCObject"],
            methods=[
                Method(
                    name="onComplete",
                    ret="void",
                    args=[],
                    is_callback=True,
                    platforms=all_platforms(),
                )
            ],
        )
        root = Root(classes=[ccobject, target])
        plan = collect_plan(root, "win")

        data = collect_audit(plan, root)
        bucket = data["buckets"]["callback_method"]

        self.assertEqual(bucket["count"], 1)
        self.assertIn("Foo.onComplete: callback", bucket["samples"])

    def test_audit_classifies_delegate_arg(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        target = Class(
            name="CCTouchDispatcher",
            bases=["CCObject"],
            methods=[
                Method(
                    name="addStandardDelegate",
                    ret="void",
                    args=[Arg("cocos2d::CCTouchDelegate*", "delegate")],
                    platforms=all_platforms(),
                )
            ],
        )
        root = Root(classes=[ccobject, target])
        plan = collect_plan(root, "win")

        data = collect_audit(plan, root)
        bucket = data["buckets"]["delegate_arg"]

        self.assertEqual(bucket["count"], 1)
        self.assertIn(
            "unsupported-arg:cocos2d::CCTouchDelegate*",
            bucket["reasonHistogram"],
        )

    def test_audit_classifies_container_arg(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        game_object = Class(name="GameObject", bases=["CCObject"])
        target = Class(
            name="GameLevelManager",
            bases=["CCObject"],
            methods=[
                Method(
                    name="loadObjects",
                    ret="void",
                    args=[Arg("gd::vector<GameObject*>*", "objects")],
                    platforms=all_platforms(),
                )
            ],
        )
        root = Root(classes=[ccobject, game_object, target])
        plan = collect_plan(root, "win")

        data = collect_audit(plan, root)
        bucket = data["buckets"]["container_arg"]

        self.assertEqual(bucket["count"], 1)
        self.assertEqual(bucket["id"], "container_arg")

    def test_audit_classifies_sel_arg(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        target = Class(
            name="CCNode",
            bases=["CCObject"],
            methods=[
                Method(
                    name="scheduleSelector",
                    ret="void",
                    args=[Arg("cocos2d::SEL_CallFunc", "selector")],
                    platforms=all_platforms(),
                )
            ],
        )
        root = Root(classes=[ccobject, target])
        plan = collect_plan(root, "win")

        data = collect_audit(plan, root)
        bucket = data["buckets"]["sel_arg"]

        self.assertEqual(bucket["count"], 1)
        self.assertEqual(bucket["id"], "sel_arg")

    def test_audit_emits_markdown_sections(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        target = Class(
            name="Foo",
            bases=["CCObject"],
            methods=[
                Method(
                    name="onComplete",
                    ret="void",
                    args=[],
                    is_callback=True,
                    platforms=all_platforms(),
                )
            ],
        )
        root = Root(classes=[ccobject, target])
        plan = collect_plan(root, "win")
        data = collect_audit(plan, root)
        md = emit_audit_markdown(data)

        self.assertIn("# LuauAPI skip audit", md)
        self.assertIn("## Summary", md)
        self.assertIn("## Samples by bucket", md)
        self.assertIn("callback_method", md)

    def test_audit_std_function_supported_not_counted(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        target = Class(
            name="PopupLayer",
            bases=["CCObject"],
            methods=[
                Method(
                    name="setHandler",
                    ret="void",
                    args=[
                        Arg("std::function<void(cocos2d::CCObject*)>", "callback"),
                    ],
                    platforms=all_platforms(),
                )
            ],
        )
        root = Root(classes=[ccobject, target])
        plan = collect_plan(root, "win")

        data = collect_audit(plan, root)
        callback_buckets = (
            "callback_method",
            "sel_arg",
            "std_function_arg",
            "callback_alias",
            "delegate_arg",
        )
        callback_skips = sum(data["buckets"][bid]["count"] for bid in callback_buckets)

        self.assertIn("setHandler", plan.supported_by_class.get("PopupLayer", []))
        self.assertEqual(callback_skips, 0)
