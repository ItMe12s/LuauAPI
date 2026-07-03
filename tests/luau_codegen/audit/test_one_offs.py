from __future__ import annotations

import unittest

from test_support import all_platforms
from luau_codegen.emit.audit import collect_audit  # type: ignore[import-unresolved]
from luau_codegen.emit.plan import collect_plan  # type: ignore[import-unresolved]
from luau_codegen.parse.broma import Arg, Class, Function, Method, Root  # type: ignore[import-unresolved]
from luau_codegen.policy.filtering import supported  # type: ignore[import-unresolved]
from luau_codegen.policy.free_functions import free_function_unsupported_reason  # type: ignore[import-unresolved]

_CALLBACK_ONE_OFFS = (
    ("ChallengesPage", "updateTimers"),
    ("DailyLevelPage", "updateTimers"),
    ("RewardsPage", "updateTimers"),
    ("HardStreak", "updateStroke"),
    ("MusicDownloadManager", "ProcessHttpRequest"),
)


class OneOffPolicyTests(unittest.TestCase):
    def test_callback_methods_rejected(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        objects: dict[str, Class] = {"CCObject": ccobject}
        for cls_name, method_name in _CALLBACK_ONE_OFFS:
            with self.subTest(cls=cls_name, method=method_name):
                cls = Class(
                    name=cls_name,
                    bases=["CCObject"],
                    attributes=["link(win, android)"],
                    methods=[
                        Method(
                            name=method_name,
                            ret="void",
                            args=[],
                            is_callback=True,
                            platforms=all_platforms(),
                        )
                    ],
                )
                objects[cls_name] = cls
                ok, reason = supported(cls, cls.methods[0], objects, "win")
                self.assertFalse(ok)
                self.assertEqual(reason, "callback")

    def test_mdpopup_void_classifies_std_function_bucket(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        popup = Class(name="Popup", bases=["CCObject"])
        target = Class(
            name="MDPopup",
            bases=["Popup"],
            attributes=["link(win, android)"],
            methods=[
                Method(
                    name="void",
                    ret="const geode::Function<void(bool)>",
                    args=[],
                    platforms=all_platforms(),
                ),
                Method(
                    name="setOnClick",
                    ret="void",
                    args=[Arg("geode::Function<void(bool)>", "handler")],
                    platforms=all_platforms(),
                ),
            ],
        )
        root = Root(classes=[ccobject, popup, target])
        plan = collect_plan(root, "win")

        data = collect_audit(plan, root)
        bucket = data["buckets"]["std_function_arg"]

        self.assertEqual(bucket["count"], 1)
        self.assertIn("MDPopup.void:", bucket["samples"][0])
        supported_names = plan.supported_by_class.get("MDPopup", [])
        self.assertNotIn("void", supported_names)
        self.assertIn("setOnClick", supported_names)

    def test_open_info_popup_unsupported_return(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        fn = Function(
            name="openInfoPopup",
            namespace="geode",
            ret="std::optional<arc::TaskHandle<bool>>",
            args=[Arg("Mod*", "mod")],
        )
        reason = free_function_unsupported_reason(fn, {"CCObject": ccobject})
        self.assertIsNotNone(reason)
        assert reason is not None
        self.assertIn("TaskHandle", reason)

    def test_open_info_popup_audit_bucket(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        fn = Function(
            name="openInfoPopup",
            namespace="geode",
            ret="std::optional<arc::TaskHandle<bool>>",
            args=[Arg("Mod*", "mod")],
        )
        root = Root(classes=[ccobject], functions=[fn])
        plan = collect_plan(root, "win")
        data = collect_audit(plan, root)
        bucket = data["buckets"]["http_async_excluded"]

        self.assertEqual(bucket["count"], 1)
        self.assertTrue(any("openInfoPopup" in sample for sample in bucket["samples"]))
