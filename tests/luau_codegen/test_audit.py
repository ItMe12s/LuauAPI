from __future__ import annotations

from conftest import *

from luau_codegen.emit.plan import EmitPlan  # type: ignore[import-unresolved]


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

    def test_audit_classifies_container_arg_for_primitive_vector(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        target = Class(
            name="Foo",
            bases=["CCObject"],
            methods=[
                Method(
                    name="takeInts",
                    ret="void",
                    args=[Arg("gd::vector<int>", "values")],
                    platforms=all_platforms(),
                )
            ],
        )
        root = Root(classes=[ccobject, target])
        plan = collect_plan(root, "win")

        data = collect_audit(plan, root)
        bucket = data["buckets"]["container_arg"]

        self.assertEqual(bucket["count"], 1)
        self.assertEqual(bucket["id"], "container_arg")

    def test_audit_supported_object_vector_method_not_in_container_bucket(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccnode = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[
                Method(
                    name="getChildren",
                    ret="gd::vector<cocos2d::CCObject*>",
                    args=[],
                    platforms=all_platforms(),
                )
            ],
        )
        root = Root(classes=[ccobject, ccnode])
        plan = collect_plan(root, "win")

        data = collect_audit(plan, root)
        bucket = data["buckets"]["container_arg"]

        self.assertEqual(bucket["count"], 0)

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

    def test_audit_classifies_std_function_arg(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        target = Class(
            name="PopupLayer",
            bases=["CCObject"],
            methods=[
                Method(
                    name="setHandler",
                    ret="void",
                    args=[Arg("std::function<bool()>", "callback")],
                    platforms=all_platforms(),
                )
            ],
        )
        root = Root(classes=[ccobject, target])
        plan = collect_plan(root, "win")

        data = collect_audit(plan, root)
        bucket = data["buckets"]["std_function_arg"]

        self.assertEqual(bucket["count"], 1)
        self.assertIn(
            "unsupported-arg:std::function<bool()>",
            bucket["reasonHistogram"],
        )

    def test_audit_classifies_callback_alias(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        target = Class(
            name="MenuLayer",
            bases=["CCObject"],
            methods=[
                Method(
                    name="registerCallback",
                    ret="void",
                    args=[Arg("Callback", "cb")],
                    platforms=all_platforms(),
                )
            ],
        )
        root = Root(classes=[ccobject, target])
        plan = collect_plan(root, "win")

        data = collect_audit(plan, root)
        bucket = data["buckets"]["callback_alias"]

        self.assertEqual(bucket["count"], 1)
        self.assertIn(
            "MenuLayer.registerCallback: unsupported-arg:Callback", bucket["samples"]
        )

    def test_audit_classifies_value_type_arg(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        target = Class(
            name="FMODAudioEngine",
            bases=["CCObject"],
            methods=[
                Method(
                    name="playSound",
                    ret="void",
                    args=[Arg("FMOD::Sound*", "sound")],
                    platforms=all_platforms(),
                )
            ],
        )
        root = Root(classes=[ccobject, target])
        plan = collect_plan(root, "win")

        data = collect_audit(plan, root)
        bucket = data["buckets"]["value_type_arg"]

        self.assertEqual(bucket["count"], 1)
        self.assertIn("unsupported-arg:FMOD::Sound*", bucket["reasonHistogram"])

    def test_audit_classifies_http_async_excluded(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        fn = Function(
            name="fetch",
            namespace="web",
            ret="geode::Task<void>",
            args=[Arg("std::string", "url")],
        )
        root = Root(classes=[ccobject], functions=[fn])
        plan = collect_plan(root, "win")

        data = collect_audit(plan, root)
        bucket = data["buckets"]["http_async_excluded"]

        self.assertEqual(bucket["count"], 1)
        self.assertIn(
            "free-function-unsupported-return:geode::Task<void>",
            bucket["reasonHistogram"],
        )
        self.assertTrue(
            any("web.fetch" in sample for sample in bucket["samples"]),
            bucket["samples"],
        )

    def test_audit_classifies_other(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        target = Class(
            name="CCGrabber",
            bases=["CCObject"],
            methods=[
                Method(
                    name="grab",
                    ret="void",
                    args=[],
                    platforms=all_platforms(),
                )
            ],
        )
        root = Root(classes=[ccobject, target])
        plan = collect_plan(root, "win")

        data = collect_audit(plan, root)
        bucket = data["buckets"]["other"]

        self.assertEqual(bucket["count"], 1)
        self.assertIn("inaccessible-class", bucket["reasonHistogram"])

    def test_audit_totals_match_plan_skips(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        root = Root(classes=[ccobject])
        plan = EmitPlan(
            classes=[ccobject],
            objects={"CCObject": ccobject},
            skipped=[
                ("Foo", "a", "callback"),
                ("Foo", "b", "unsupported-arg:gd::vector<int>"),
            ],
            skipped_by_class={},
            skipped_classes=set(),
            supported_by_class={},
            hook_targets_by_class={},
            field_targets_by_class={},
            depths={},
            skipped_free_functions=[
                (
                    "web::fetch()",
                    "web",
                    "fetch",
                    "free-function-unsupported-return:geode::Task<void>",
                ),
            ],
        )

        data = collect_audit(plan, root)

        self.assertEqual(data["totalSkippedMethods"], 2)
        self.assertEqual(data["totalSkippedFreeFunctions"], 1)

    def test_audit_markdown_omits_empty_buckets(self) -> None:
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
        md = emit_audit_markdown(collect_audit(plan, root))

        self.assertIn("callback_method", md)
        self.assertNotIn("| delegate_arg |", md)
        self.assertNotIn("### delegate_arg", md)

    def test_audit_markdown_sample_cap(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        root = Root(classes=[ccobject])
        skipped = [
            (
                f"GameLevelManager",
                f"load{i}",
                "unsupported-arg:gd::vector<int>",
            )
            for i in range(30)
        ]
        plan = EmitPlan(
            classes=[ccobject],
            objects={"CCObject": ccobject},
            skipped=skipped,
            skipped_by_class={},
            skipped_classes=set(),
            supported_by_class={},
            hook_targets_by_class={},
            field_targets_by_class={},
            depths={},
        )

        md = emit_audit_markdown(collect_audit(plan, root))

        self.assertIn("... 5 more", md)
        self.assertEqual(md.count("GameLevelManager.load"), 25)
