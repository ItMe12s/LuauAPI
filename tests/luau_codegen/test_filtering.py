from __future__ import annotations

from conftest import *


class LinkClassFilterTests(unittest.TestCase):
    def test_missing_address_rejected_without_class_link(self) -> None:
        cls = Class(name="Foo")
        method = Method(name="bar", ret="void", args=[])

        ok, reason = supported(cls, method, {}, "win")

        self.assertFalse(ok)
        self.assertEqual(reason, "missing-address")

    def test_missing_address_allowed_for_link_class(self) -> None:
        cls = Class(name="CCScheduler", attributes=["link(win, android)"])
        method = Method(name="update", ret="void", args=[Arg("float", "dt")])

        ok, reason = supported(cls, method, {"CCScheduler": cls}, "win")

        self.assertTrue(ok)
        self.assertEqual(reason, "")

    def test_missing_address_still_rejected_for_unlinked_platform(self) -> None:
        cls = Class(name="CCScheduler", attributes=["link(android)"])
        method = Method(name="update", ret="void", args=[Arg("float", "dt")])

        ok, reason = supported(cls, method, {"CCScheduler": cls}, "win")

        self.assertFalse(ok)
        self.assertEqual(reason, "missing-address")

    def test_missing_platform_rejected_on_win(self) -> None:
        cls = Class(name="TestLinkClass", attributes=["link(win, android)"])
        method = Method(
            name="setupVBO",
            ret="void",
            args=[],
            attributes=["missing(win, android)"],
        )

        ok, reason = supported(cls, method, {"TestLinkClass": cls}, "win")

        self.assertFalse(ok)
        self.assertEqual(reason, "missing-platform")

    def test_missing_platform_allowed_on_mac(self) -> None:
        cls = Class(name="TestLinkClass", attributes=["link(win, mac)"])
        method = Method(
            name="setupVBO",
            ret="void",
            args=[],
            attributes=["missing(win, android)"],
        )

        ok, reason = supported(cls, method, {"TestLinkClass": cls}, "mac")

        self.assertTrue(ok)
        self.assertEqual(reason, "")

    def test_underscore_internal_rejected_on_link_class(self) -> None:
        cls = Class(name="CCImage", attributes=["link(win, android)"])
        method = Method(
            name="_saveImageToJPG", ret="bool", args=[Arg("char const*", "path")]
        )

        ok, reason = supported(cls, method, {"CCImage": cls}, "win")

        self.assertFalse(ok)
        self.assertEqual(reason, "inaccessible")

    def test_game_level_manager_has_liked_item_full_check_is_denied(self) -> None:
        cls = Class(name="GameLevelManager", attributes=["link(android)"])
        method = Method(
            name="hasLikedItemFullCheck",
            ret="bool",
            args=[
                Arg("LikeItemType", "type"),
                Arg("int", "id"),
                Arg("bool", "liked"),
                Arg("int", "parentID"),
            ],
            platforms={
                "win": "0x164d80",
                "imac": "0x557a90",
                "m1": "0x4a78e0",
                "ios": "0xaa3f4",
            },
        )

        ok, reason = supported(
            cls,
            method,
            {"GameLevelManager": cls, "LikeItemType": Class(name="LikeItemType")},
            "android64",
        )

        self.assertFalse(ok)
        self.assertEqual(reason, "inaccessible")


class SelMenuHandlerFilterTests(unittest.TestCase):
    def test_orphan_sel_menu_handler_is_rejected(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(name="Foo", namespace="cocos2d", bases=["CCObject"])
        method = Method(
            name="register",
            ret="void",
            args=[
                Arg("char const*", "label"),
                Arg("SEL_MenuHandler", "selector"),
            ],
            platforms=all_platforms("0x1"),
        )
        objects = {"CCObject": ccobject, "Foo": cls}

        ok, reason = supported(cls, method, objects, "win")

        self.assertFalse(ok)
        self.assertEqual(reason, "unsupported-arg:SEL_MenuHandler")

    def test_valid_sel_pair_is_accepted(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(name="CCMenuItem", namespace="cocos2d", bases=["CCNode"])
        method = Method(
            name="initWithTarget",
            ret="bool",
            args=[
                Arg("cocos2d::CCObject*", "target"),
                Arg("SEL_MenuHandler", "selector"),
            ],
            platforms=all_platforms("0x1"),
        )
        ccnode = Class(name="CCNode", namespace="cocos2d", bases=["CCObject"])
        objects = {
            "CCObject": ccobject,
            "CCNode": ccnode,
            "CCMenuItem": cls,
            "cocos2d::CCObject": ccobject,
            "cocos2d::CCNode": ccnode,
            "cocos2d::CCMenuItem": cls,
        }

        ok, reason = supported(cls, method, objects, "win")

        self.assertTrue(ok)
        self.assertEqual(reason, "")


class FreeFunctionSelMenuHandlerFilterTests(unittest.TestCase):
    def test_orphan_sel_menu_handler_is_rejected(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        fn = Function(
            name="register",
            namespace="geode::utils",
            ret="void",
            args=[Arg("SEL_MenuHandler", "selector")],
        )
        objects = {"CCObject": ccobject}

        reason = free_function_unsupported_reason(fn, objects)

        self.assertEqual(reason, "free-function-unsupported-arg:SEL_MenuHandler")

    def test_valid_sel_pair_is_accepted(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        fn = Function(
            name="attachHandler",
            namespace="geode::utils",
            ret="void",
            args=[
                Arg("cocos2d::CCObject*", "target"),
                Arg("SEL_MenuHandler", "selector"),
            ],
        )
        objects = {"CCObject": ccobject, "cocos2d::CCObject": ccobject}

        reason = free_function_unsupported_reason(fn, objects)

        self.assertIsNone(reason)
