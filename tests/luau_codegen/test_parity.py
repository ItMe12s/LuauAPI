from __future__ import annotations

import unittest
from unittest import mock
from helpers import (
    Arg,  # type: ignore[import-unresolved]
    Class,  # type: ignore[import-unresolved]
    Function,  # type: ignore[import-unresolved]
    Method,  # type: ignore[import-unresolved]
    Root,  # type: ignore[import-unresolved]
    all_platforms,  # type: ignore[import-unresolved]
    collect_parity,  # type: ignore[import-unresolved]
    collect_plan,  # type: ignore[import-unresolved]
    collect_platform_plan,  # type: ignore[import-unresolved]
    emit_luau_types,  # type: ignore[import-unresolved]
    emit_markdown,  # type: ignore[import-unresolved]
    free_function_key,  # type: ignore[import-unresolved]
    plan_outputs,  # type: ignore[import-unresolved]
    types_text,  # type: ignore[import-unresolved]
)


class F12ParityReportTests(unittest.TestCase):
    def test_parity_reports_free_function_intersection_drop(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        fn = Function(
            name="getEnvironmentVariable",
            namespace="geode::utils",
            ret="std::string",
            args=[Arg("ZStringView", "name")],
        )
        root = Root(classes=[ccobject], functions=[fn])

        data = collect_parity(root)
        key = free_function_key(fn)

        self.assertIn("win", data["freeFunctions"][key]["supportedPlatforms"])
        self.assertNotIn("ios", data["freeFunctions"][key]["supportedPlatforms"])
        self.assertEqual(
            data["freeFunctions"][key]["skipReasons"]["ios"],
            "free-function-excluded:ios",
        )
        self.assertEqual(data["intersection"]["freeFunctionsRemoved"], 1)
        self.assertIn("common free functions", emit_markdown(data))

    def test_parity_reports_final_free_function_skip_reason(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        weak = Class(
            name="WeakFreeFnType",
            bases=["CCObject"],
            methods=[
                Method(name="winOnly", ret="void", args=[], platforms={"win": "0x1"}),
                Method(name="m1Only", ret="void", args=[], platforms={"m1": "0x1"}),
                Method(name="iosOnly", ret="void", args=[], platforms={"ios": "0x1"}),
                Method(
                    name="android32Only",
                    ret="void",
                    args=[],
                    platforms={"android32": "0x1"},
                ),
                Method(
                    name="android64Only",
                    ret="void",
                    args=[],
                    platforms={"android64": "0x1"},
                ),
            ],
        )
        fn = Function(
            name="makeWeak",
            namespace="geode::utils",
            ret="WeakFreeFnType*",
            args=[],
        )
        root = Root(classes=[ccobject, weak], functions=[fn])

        data = collect_parity(root)
        key = free_function_key(fn)

        self.assertEqual(
            data["freeFunctions"][key]["supportedPlatforms"],
            ["win", "m1", "ios", "android32", "android64"],
        )
        self.assertEqual(
            data["freeFunctions"][key]["finalSkipReason"],
            "not-callable-type:win:WeakFreeFnType",
        )

    def test_intersection_keeps_only_all_platform_methods(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        foo = Class(
            name="Foo",
            bases=["CCObject"],
            methods=[
                Method(name="shared", ret="void", args=[], platforms=all_platforms()),
                Method(
                    name="missingIOS",
                    ret="void",
                    args=[],
                    platforms={
                        "win": "0x1",
                        "m1": "0x1",
                        "android32": "0x1",
                        "android64": "0x1",
                    },
                ),
            ],
        )
        root = Root(classes=[ccobject, foo])

        plan = collect_plan(root, "win")

        self.assertIn("shared", plan.supported_by_class["Foo"])
        self.assertNotIn("missingIOS", plan.supported_by_class["Foo"])
        self.assertTrue(
            any(
                cls == "Foo"
                and method == "missingIOS"
                and reason == "intersection-missing-platform:ios"
                for cls, method, reason in plan.skipped
            )
        )

    def test_intersection_removes_hooks_missing_one_platform(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        foo = Class(
            name="Foo",
            bases=["CCObject"],
            methods=[
                Method(
                    name="hookMe",
                    ret="void",
                    args=[],
                    platforms={
                        "win": "0x1",
                        "m1": "0x1",
                        "ios": "0x1",
                        "android32": "inline",
                        "android64": "inline",
                    },
                )
            ],
        )
        root = Root(classes=[ccobject, foo])

        plan = collect_plan(root, "win")

        self.assertIn("hookMe", plan.supported_by_class["Foo"])
        self.assertEqual(plan.hook_targets_by_class["Foo"], [])
        self.assertEqual(len(plan.intersection_stats.removed_hooks), 1)

    def test_type_factories_use_intersection_plan(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        factory = Class(
            name="FactoryOnly",
            bases=["CCObject"],
            methods=[
                Method(
                    name="create",
                    ret="FactoryOnly*",
                    args=[],
                    is_static=True,
                    platforms={
                        "win": "0x1",
                        "m1": "0x1",
                        "android32": "0x1",
                        "android64": "0x1",
                    },
                )
            ],
        )
        root = Root(classes=[ccobject, factory])

        files = emit_luau_types(root)

        self.assertNotIn("FactoryOnlyFactory", types_text(files))

    def test_class_without_intersected_members_has_no_binding_file(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        weak = Class(
            name="WeakClass",
            bases=["CCObject"],
            methods=[
                Method(
                    name="winOnly",
                    ret="void",
                    args=[],
                    platforms={"win": "0x1"},
                )
            ],
        )
        root = Root(classes=[ccobject, weak])

        outputs = plan_outputs(root, "win")
        files = emit_luau_types(root)

        self.assertNotIn("bindings_WeakClass.cpp", outputs)
        self.assertNotIn("WeakClass", types_text(files))

    def test_android_link_class_reports_missing_win_callable_proof(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        android_only = Class(
            name="AndroidOnly",
            bases=["CCObject"],
            attributes=["link(android)"],
            methods=[Method(name="ping", ret="void", args=[])],
        )
        root = Root(classes=[ccobject, android_only])

        data = collect_parity(root)
        key = "AndroidOnly.ping()"

        self.assertIn("android64", data["methods"][key]["supportedPlatforms"])
        self.assertEqual(data["methods"][key]["skipReasons"]["win"], "missing-address")
        self.assertIn(key, data["hints"]["winMissingCallableProof"])

    def test_m1_offset_method_reports_supported(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        m1_only = Class(
            name="M1Only",
            bases=["CCObject"],
            methods=[
                Method(
                    name="tick",
                    ret="void",
                    args=[],
                    platforms={"m1": "0x1234"},
                )
            ],
        )
        root = Root(classes=[ccobject, m1_only])

        data = collect_parity(root)
        key = "M1Only.tick()"

        self.assertIn("m1", data["methods"][key]["supportedPlatforms"])

    def test_collect_parity_reuses_supplied_platform_plans(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        foo = Class(
            name="Foo",
            namespace="gd",
            bases=["CCObject"],
            methods=[Method(name="shared", ret="void", args=[], platforms=all_platforms())],
        )
        root = Root(classes=[ccobject, foo])
        platforms = ("win", "m1", "ios", "android32", "android64")
        plans = {platform: collect_platform_plan(root, platform) for platform in platforms}

        with mock.patch("luau_codegen.emit.parity.collect_platform_plan") as mocked_collect:
            data = collect_parity(root, platforms=platforms, plans_by_platform=plans)

        mocked_collect.assert_not_called()
        self.assertIn("gd::Foo.shared()", data["methods"])

    def test_ios_strict_callable_class_prune_is_reported(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        static_only = Class(
            name="StaticOnly",
            bases=["CCObject"],
            methods=[
                Method(
                    name="make",
                    ret="StaticOnly*",
                    args=[],
                    is_static=True,
                    platforms={"inline": "inline"},
                )
            ],
        )
        root = Root(classes=[ccobject, static_only])

        data = collect_parity(root)
        key = "StaticOnly.make()"

        self.assertIn("StaticOnly", data["skippedClasses"]["ios"])
        self.assertEqual(
            data["methods"][key]["skipReasons"]["ios"],
            "not-callable-type:ios:StaticOnly",
        )
        self.assertIn("StaticOnly", emit_markdown(data))

    def test_inaccessible_class_method_ref_is_pruned(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccnode = Class(name="CCNode", namespace="cocos2d", bases=["CCObject"])
        batch_node = Class(
            name="CCParticleBatchNode",
            namespace="cocos2d",
            bases=["CCNode"],
        )
        foo = Class(
            name="Foo",
            namespace="cocos2d",
            bases=["CCNode"],
            methods=[
                Method(
                    name="getBatch",
                    ret="CCParticleBatchNode*",
                    args=[],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        root = Root(classes=[ccobject, ccnode, batch_node, foo])

        data = collect_parity(root)
        key = "cocos2d::Foo.getBatch()"

        self.assertEqual(
            data["methods"][key]["skipReasons"]["win"],
            "not-callable-type:win:CCParticleBatchNode",
        )
        self.assertEqual(
            data["methods"][key]["skipReasons"]["ios"],
            "not-callable-type:ios:CCParticleBatchNode",
        )

    def test_hook_parity_separates_callable_from_hookable(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        inline_only = Class(
            name="InlineOnly",
            bases=["CCObject"],
            methods=[
                Method(
                    name="localCall",
                    ret="void",
                    args=[],
                    platforms={"inline": "inline"},
                )
            ],
        )
        root = Root(classes=[ccobject, inline_only])

        data = collect_parity(root)
        key = "InlineOnly.localCall()"

        self.assertIn("win", data["methods"][key]["supportedPlatforms"])
        self.assertNotIn("win", data["methods"][key]["hookablePlatforms"])
        self.assertIn(key, data["hints"]["hookAddressGaps"])
