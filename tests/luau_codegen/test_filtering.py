from __future__ import annotations

import unittest
from helpers import (
    Arg,  # type: ignore[import-unresolved]
    Class,  # type: ignore[import-unresolved]
    Function,  # type: ignore[import-unresolved]
    Method,  # type: ignore[import-unresolved]
    all_platforms,  # type: ignore[import-unresolved]
    free_function_unsupported_reason,  # type: ignore[import-unresolved]
    supported,  # type: ignore[import-unresolved]
)


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
        method = Method(name="_saveImageToJPG", ret="bool", args=[Arg("char const*", "path")])

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

    def test_cctouch_dispatcher_protected_helpers_denied(self) -> None:
        cls = Class(name="CCTouchDispatcher", namespace="cocos2d")
        objects = {"CCTouchDispatcher": cls, "cocos2d::CCTouchDispatcher": cls}
        for name in ("findHandler", "forceRemoveDelegate"):
            method = Method(
                name=name,
                ret=("void" if name == "forceRemoveDelegate" else "cocos2d::CCTouchHandler*"),
                args=[Arg("cocos2d::CCTouchDelegate*", "delegate")],
            )
            ok, reason = supported(cls, method, objects, "win")
            self.assertFalse(ok, name)
            self.assertEqual(reason, "inaccessible", name)

    def test_cccontrol_picker_protected_touch_handlers_denied(self) -> None:
        protected = [
            ("CCControlColourPicker", "ccTouchBegan", "bool"),
            ("CCControlHuePicker", "ccTouchBegan", "bool"),
            ("CCControlHuePicker", "ccTouchMoved", "void"),
            ("CCControlSaturationBrightnessPicker", "ccTouchBegan", "bool"),
            ("CCControlSaturationBrightnessPicker", "ccTouchMoved", "void"),
        ]
        for class_name, method_name, ret in protected:
            cls = Class(
                name=class_name,
                namespace="cocos2d::extension",
                attributes=["link(win, android)"],
            )
            method = Method(
                name=method_name,
                ret=ret,
                args=[
                    Arg("cocos2d::CCTouch*", "touch"),
                    Arg("cocos2d::CCEvent*", "event"),
                ],
                platforms={"imac": "0x1", "m1": "0x2", "ios": "0x3"},
            )
            ok, reason = supported(cls, method, {class_name: cls}, "m1")
            self.assertFalse(ok, f"{class_name}::{method_name}")
            self.assertEqual(reason, "inaccessible", f"{class_name}::{method_name}")

    def test_touch_handler_impl_classes_are_inaccessible(self) -> None:
        for name in ("CCStandardTouchHandler", "CCTargetedTouchHandler"):
            cls = Class(
                name=name,
                namespace="cocos2d",
                attributes=["link(win, android)"],
            )
            method = Method(
                name="handlerWithDelegate",
                ret=f"{name}*",
                is_static=True,
                args=[
                    Arg("cocos2d::CCTouchDelegate*", "delegate"),
                    Arg("int", "priority"),
                ],
                platforms={"m1": "0x1", "imac": "0x2", "ios": "0x3"},
            )
            ok, reason = supported(cls, method, {name: cls}, "m1")
            self.assertFalse(ok, name)
            self.assertEqual(reason, "inaccessible-class", name)


class SelMenuHandlerFilterTests(unittest.TestCase):
    def test_orphan_sel_menu_handler_is_accepted(self) -> None:
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

        self.assertTrue(ok)
        self.assertEqual(reason, "")

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


class ContainerFilterTests(unittest.TestCase):
    def test_vector_return_method_is_supported(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(name="CCNode", namespace="cocos2d", bases=["CCObject"])
        method = Method(
            name="getChildren",
            ret="gd::vector<cocos2d::CCObject*>",
            args=[],
            platforms=all_platforms("0x1"),
        )
        objects = {"CCObject": ccobject, "cocos2d::CCObject": ccobject, "CCNode": cls}

        ok, reason = supported(cls, method, objects, "win")

        self.assertTrue(ok)
        self.assertEqual(reason, "")

    def test_vector_const_ref_return_method_is_supported(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(name="CCNode", namespace="cocos2d", bases=["CCObject"])
        method = Method(
            name="getChildrenRef",
            ret="gd::vector<cocos2d::CCObject*> const&",
            args=[],
            platforms=all_platforms("0x1"),
        )
        objects = {"CCObject": ccobject, "cocos2d::CCObject": ccobject, "CCNode": cls}

        ok, reason = supported(cls, method, objects, "win")

        self.assertTrue(ok)
        self.assertEqual(reason, "")

    def test_vector_input_arg_method_is_supported(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(name="CCNode", namespace="cocos2d", bases=["CCObject"])
        method = Method(
            name="setChildren",
            ret="void",
            args=[Arg("gd::vector<cocos2d::CCObject*> const&", "children")],
            platforms=all_platforms("0x1"),
        )
        objects = {"CCObject": ccobject, "cocos2d::CCObject": ccobject, "CCNode": cls}

        ok, reason = supported(cls, method, objects, "win")

        self.assertTrue(ok)
        self.assertEqual(reason, "")

    def test_vector_out_arg_requires_void_return(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        game_object = Class(name="GameObject", bases=["CCObject"])
        cls = Class(name="GameLevelManager", bases=["CCObject"])
        method = Method(
            name="loadObjects",
            ret="bool",
            args=[Arg("gd::vector<GameObject*>*", "objects")],
            platforms=all_platforms("0x1"),
        )
        objects = {
            "CCObject": ccobject,
            "GameObject": game_object,
            "GameLevelManager": cls,
        }

        ok, reason = supported(cls, method, objects, "win")

        self.assertFalse(ok)
        self.assertEqual(reason, "unsupported-arg:gd::vector<GameObject*>*")

    def test_vector_out_arg_void_return_is_supported(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        game_object = Class(name="GameObject", bases=["CCObject"])
        cls = Class(name="GameLevelManager", bases=["CCObject"])
        method = Method(
            name="loadObjects",
            ret="void",
            args=[Arg("gd::vector<GameObject*>*", "objects")],
            platforms=all_platforms("0x1"),
        )
        objects = {
            "CCObject": ccobject,
            "GameObject": game_object,
            "GameLevelManager": cls,
        }

        ok, reason = supported(cls, method, objects, "win")

        self.assertTrue(ok)
        self.assertEqual(reason, "")

    def test_primitive_vector_input_arg_is_supported(self) -> None:
        cls = Class(name="Foo")
        method = Method(
            name="take",
            ret="void",
            args=[Arg("gd::vector<int>", "values")],
            platforms=all_platforms("0x1"),
        )

        ok, reason = supported(cls, method, {}, "win")

        self.assertTrue(ok)
        self.assertEqual(reason, "")

    def test_primitive_vector_return_is_supported(self) -> None:
        cls = Class(name="Foo")
        method = Method(
            name="getValues",
            ret="gd::vector<int>",
            args=[],
            platforms=all_platforms("0x1"),
        )

        ok, reason = supported(cls, method, {}, "win")

        self.assertTrue(ok)
        self.assertEqual(reason, "")

    def test_primitive_vector_out_arg_void_return_is_supported(self) -> None:
        cls = Class(name="Foo")
        method = Method(
            name="fillValues",
            ret="void",
            args=[Arg("gd::vector<int>*", "values")],
            platforms=all_platforms("0x1"),
        )

        ok, reason = supported(cls, method, {}, "win")

        self.assertTrue(ok)
        self.assertEqual(reason, "")

    def test_primitive_vector_out_arg_non_void_return_stays_unsupported(self) -> None:
        cls = Class(name="Foo")
        method = Method(
            name="fillValues",
            ret="bool",
            args=[Arg("gd::vector<int>*", "values")],
            platforms=all_platforms("0x1"),
        )

        ok, reason = supported(cls, method, {}, "win")

        self.assertFalse(ok)
        self.assertEqual(reason, "unsupported-arg:gd::vector<int>*")

    def test_map_input_arg_is_supported(self) -> None:
        cls = Class(name="Foo")
        method = Method(
            name="takeMap",
            ret="void",
            args=[Arg("gd::map<int, bool>", "values")],
            platforms=all_platforms("0x1"),
        )

        ok, reason = supported(cls, method, {}, "win")

        self.assertTrue(ok)
        self.assertEqual(reason, "")

    def test_leading_const_container_ref_input_arg_is_supported(self) -> None:
        cls = Class(name="Foo")
        method = Method(
            name="takeUsedIds",
            ret="int",
            args=[Arg("const gd::unordered_set<int>&", "used")],
            platforms=all_platforms("0x1"),
        )

        ok, reason = supported(cls, method, {}, "win")

        self.assertTrue(ok)
        self.assertEqual(reason, "")

    def test_map_return_is_supported(self) -> None:
        cls = Class(name="Foo")
        method = Method(
            name="getMap",
            ret="gd::map<int, bool>",
            args=[],
            platforms=all_platforms("0x1"),
        )

        ok, reason = supported(cls, method, {}, "win")

        self.assertTrue(ok)
        self.assertEqual(reason, "")

    def test_set_input_arg_is_supported(self) -> None:
        cls = Class(name="Foo")
        method = Method(
            name="takeSet",
            ret="void",
            args=[Arg("gd::set<int>", "values")],
            platforms=all_platforms("0x1"),
        )

        ok, reason = supported(cls, method, {}, "win")

        self.assertTrue(ok)
        self.assertEqual(reason, "")

    def test_map_out_arg_non_void_return_stays_unsupported(self) -> None:
        cls = Class(name="Foo")
        method = Method(
            name="fillMap",
            ret="bool",
            args=[Arg("gd::map<int, bool>*", "values")],
            platforms=all_platforms("0x1"),
        )

        ok, reason = supported(cls, method, {}, "win")

        self.assertFalse(ok)
        self.assertEqual(reason, "unsupported-arg:gd::map<int, bool>*")


class FmodFilterTests(unittest.TestCase):
    def _fmod_engine(self) -> tuple[Class, dict[str, Class]]:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="FMODAudioEngine",
            bases=["CCObject"],
            attributes=["link(win)"],
        )
        objects = {"CCObject": ccobject, "FMODAudioEngine": cls}
        return cls, objects

    def test_fmod_enum_methods_are_supported(self) -> None:
        cls, objects = self._fmod_engine()
        methods = [
            Method(
                name="printResult",
                ret="void",
                args=[Arg("FMOD_RESULT", "result")],
                platforms=all_platforms("0x1"),
            ),
            Method(
                name="waitUntilSoundReady",
                ret="FMOD_OPENSTATE",
                args=[],
                platforms=all_platforms("0x2"),
            ),
            Method(
                name="setSoftwareFormat",
                ret="FMOD_RESULT",
                args=[
                    Arg("int", "sampleRate"),
                    Arg("FMOD_SPEAKERMODE", "speakerMode"),
                    Arg("int", "numRawSpeakers"),
                ],
                platforms=all_platforms("0x3"),
            ),
        ]
        for method in methods:
            ok, reason = supported(cls, method, objects, "win")
            self.assertTrue(ok, reason)
            self.assertEqual(reason, "")

    def test_fmod_opaque_handle_methods_are_supported(self) -> None:
        cls, objects = self._fmod_engine()
        methods = [
            Method(
                name="getActiveMusicChannel",
                ret="FMOD::Channel*",
                args=[],
                platforms=all_platforms("0x1"),
            ),
            Method(
                name="isSoundReady",
                ret="bool",
                args=[Arg("FMOD::Sound*", "sound")],
                platforms=all_platforms("0x2"),
            ),
            Method(
                name="getChannelGroup",
                ret="FMOD::ChannelGroup*",
                args=[],
                platforms=all_platforms("0x3"),
            ),
            Method(
                name="channelLinkSound",
                ret="void",
                args=[Arg("int", "id"), Arg("FMODSound*", "sound")],
                platforms=all_platforms("0x4"),
            ),
            Method(
                name="preloadEffect",
                ret="FMODSound*",
                args=[Arg("gd::string", "path")],
                platforms=all_platforms("0x5"),
            ),
            Method(
                name="storeEffect",
                ret="FMODSound*",
                args=[Arg("FMOD::Sound*", "sound"), Arg("gd::string", "path")],
                platforms=all_platforms("0x6"),
            ),
        ]
        for method in methods:
            ok, reason = supported(cls, method, objects, "win")
            self.assertTrue(ok, reason)
            self.assertEqual(reason, "")

    def test_unlisted_fmod_pointer_stays_unsupported(self) -> None:
        cls, objects = self._fmod_engine()
        method = Method(
            name="takeDSP",
            ret="void",
            args=[Arg("FMOD::DSP*", "dsp")],
            platforms=all_platforms("0x4"),
        )
        ok, reason = supported(cls, method, objects, "win")
        self.assertFalse(ok)
        self.assertEqual(reason, "unsupported-arg:FMOD::DSP*")


class FreeFunctionSelMenuHandlerFilterTests(unittest.TestCase):
    def test_orphan_sel_menu_handler_is_accepted(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        fn = Function(
            name="register",
            namespace="geode::utils",
            ret="void",
            args=[Arg("SEL_MenuHandler", "selector")],
        )
        objects = {"CCObject": ccobject}

        reason = free_function_unsupported_reason(fn, objects)

        self.assertIsNone(reason)

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
