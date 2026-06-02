from __future__ import annotations

from conftest import *


class GeodeEnumRegistrationTests(unittest.TestCase):
    def setUp(self) -> None:
        self._saved_geode_enums = set(GEODE_ENUM_TYPES)
        self._saved_enum_types = frozenset(type_map_module.ENUM_TYPES)
        self._saved_enum_cxx = dict(type_map_module.ENUM_CXX_NAMES)

    def tearDown(self) -> None:
        GEODE_ENUM_TYPES.clear()
        GEODE_ENUM_TYPES.update(self._saved_geode_enums)
        type_map_module.ENUM_CXX_NAMES.clear()
        type_map_module.ENUM_CXX_NAMES.update(self._saved_enum_cxx)
        type_map_module.ENUM_TYPES = self._saved_enum_types

    def test_register_geode_enums_skips_reserved(self) -> None:
        skip = GD_ENUM_TYPES | COCOS_ENUM_TYPES | {"CollidingClass"}
        register_geode_enums(
            {
                "IconType": "IconType",
                "enumKeyCodes": "cocos2d::enumKeyCodes",
                "CollidingClass": "geode::CollidingClass",
                "UniqueGeodeEnum": "geode::UniqueGeodeEnum",
            },
            skip=skip,
        )
        self.assertNotIn("IconType", GEODE_ENUM_TYPES)
        self.assertNotIn("enumKeyCodes", GEODE_ENUM_TYPES)
        self.assertNotIn("CollidingClass", GEODE_ENUM_TYPES)
        self.assertIn("UniqueGeodeEnum", GEODE_ENUM_TYPES)

    def test_register_geode_enums_keeps_unique(self) -> None:
        register_geode_enums({"BaseType": "geode::BaseType"})
        self.assertEqual(GEODE_ENUM_TYPES, {"BaseType"})
        self.assertIn("BaseType", type_map_module.ENUM_CXX_NAMES)

    def test_classify_arg_no_enum_shadow(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        colliding = Class(name="CollidingClass", namespace="geode", bases=["CCObject"])
        root = Root(classes=[ccobject, colliding])
        objects = codegen_object_map(root)
        skip = GD_ENUM_TYPES | COCOS_ENUM_TYPES | {c.name for c in object_classes(root)}
        register_geode_enums({"CollidingClass": "geode::CollidingClass"}, skip=skip)
        info = classify_arg("CollidingClass*", objects)
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "object")
        self.assertEqual(info.class_name, "CollidingClass")

    def test_classify_sel_menu_handler(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        objects = {"CCObject": ccobject}
        info = classify_arg("SEL_MenuHandler", objects)
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "sel")
        self.assertEqual(info.lua_type, "(sender: CCObject) -> ()")

    def test_classify_std_function_callback(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        objects = {"CCObject": ccobject}
        info = classify_arg("std::function<void(cocos2d::CCObject*)>", objects)
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "callback")
        self.assertEqual(info.lua_type, "(arg1: CCObject) -> ()")

    def test_classify_std_function_non_void_return(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        objects = {"CCObject": ccobject}
        info = classify_arg("std::function<bool(cocos2d::CCObject*)>", objects)
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "callback")
        self.assertEqual(info.lua_type, "(arg1: CCObject) -> boolean")
        assert info.callback_ret is not None
        self.assertEqual(info.callback_ret.kind, "bool")

    def test_classify_callback_alias(self) -> None:
        info = classify_arg("Callback", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "callback")
        self.assertEqual(info.lua_type, "() -> ()")

    def test_classify_lazy_sprite_callback_alias(self) -> None:
        info = classify_arg("Callback", {}, owner_class="LazySprite")
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "callback")
        self.assertEqual(info.lua_type, "(arg1: boolean | string) -> ()")
        assert info.callback_args
        self.assertEqual(info.callback_args[0].kind, "result")

    def test_classify_geode_result_void(self) -> None:
        info = classify_arg("geode::Result<>", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "result")
        self.assertEqual(info.lua_type, "boolean | string")

    def test_classify_sel_schedule(self) -> None:
        info = classify_arg("SEL_SCHEDULE", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "sel")
        self.assertEqual(info.lua_type, "(dt: number) -> ()")
        self.assertEqual(info.class_name, "schedule")

    def test_classify_cctouch_delegate(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        objects = {"CCObject": ccobject}
        info = classify_arg("cocos2d::CCTouchDelegate*", objects)
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "delegate")
        self.assertEqual(info.class_name, "CCTouchDelegate")
        self.assertIn("ccTouchBegan", info.lua_type)

    def test_classify_cc_director_delegate(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        objects = {"CCObject": ccobject}
        info = classify_arg("cocos2d::CCDirectorDelegate*", objects)
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "delegate")
        self.assertEqual(info.class_name, "CCDirectorDelegate")
        self.assertIn("updateProjection", info.lua_type)

    def test_classify_cccolor4b_value(self) -> None:
        info = classify_arg("cocos2d::ccColor4B const&", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "value")
        self.assertEqual(info.lua_type, "RGBAColor")

    def test_method_input_arg_count_collapses_sel_pair(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccnode = Class(name="CCNode", namespace="cocos2d", bases=["CCObject"])
        ccsprite = Class(name="CCSprite", namespace="cocos2d", bases=["CCObject"])
        menu_item = Class(name="CCMenuItem", namespace="cocos2d", bases=["CCNode"])
        method = Method(
            name="initWithNormalSprite",
            ret="bool",
            args=[
                Arg("cocos2d::CCSprite*", "normal"),
                Arg("cocos2d::CCSprite*", "selected"),
                Arg("cocos2d::CCObject*", "target"),
                Arg("SEL_MenuHandler", "selector"),
            ],
        )
        objects = {
            "CCObject": ccobject,
            "CCNode": ccnode,
            "CCSprite": ccsprite,
            "CCMenuItem": menu_item,
            "cocos2d::CCObject": ccobject,
            "cocos2d::CCNode": ccnode,
            "cocos2d::CCSprite": ccsprite,
            "cocos2d::CCMenuItem": menu_item,
        }

        self.assertEqual(_input_arg_count(method, objects), 3)

    def test_method_input_arg_count_lazy_sprite_callback_uses_owner_class(
        self,
    ) -> None:
        method = Method(
            name="setLoadCallback",
            ret="void",
            args=[Arg("Callback", "cb")],
        )
        objects: dict[str, Class] = {}

        self.assertEqual(_input_arg_count(method, objects, owner_class="LazySprite"), 1)
        info = classify_arg("Callback", objects, owner_class="LazySprite")
        assert info is not None
        self.assertEqual(len(info.callback_args), 1)


class ContainerTypeMapTests(unittest.TestCase):
    def test_classify_gd_vector_object_pointer_as_readonly_view(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        objects = {"CCObject": ccobject, "cocos2d::CCObject": ccobject}

        info = classify_arg("gd::vector<cocos2d::CCObject*>", objects)

        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "vector_view")
        self.assertEqual(info.class_name, "CCObject")
        self.assertEqual(info.lua_type, "{ CCObject? }")
        self.assertFalse(info.is_ref)
        self.assertFalse(info.is_out)
        self.assertFalse(info.is_vector_ptr)

    def test_classify_gd_vector_const_ref(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        objects = {"CCObject": ccobject, "cocos2d::CCObject": ccobject}

        info = classify_arg("gd::vector<cocos2d::CCObject*> const&", objects)

        assert info is not None
        self.assertEqual(info.kind, "vector_view")
        self.assertTrue(info.is_ref)
        self.assertFalse(info.is_out)
        self.assertFalse(info.is_vector_ptr)

    def test_classify_gd_vector_out_ref(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        objects = {"CCObject": ccobject, "cocos2d::CCObject": ccobject}

        info = classify_arg("gd::vector<cocos2d::CCObject*>&", objects)

        assert info is not None
        self.assertEqual(info.kind, "vector_view")
        self.assertTrue(info.is_ref)
        self.assertTrue(info.is_out)
        self.assertFalse(info.is_vector_ptr)

    def test_classify_gd_vector_pointer_out(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        game_object = Class(name="GameObject", bases=["CCObject"])
        objects = {
            "CCObject": ccobject,
            "GameObject": game_object,
            "cocos2d::CCObject": ccobject,
        }

        info = classify_arg("gd::vector<GameObject*>*", objects)

        assert info is not None
        self.assertEqual(info.kind, "vector_view")
        self.assertFalse(info.is_ref)
        self.assertTrue(info.is_out)
        self.assertTrue(info.is_vector_ptr)
        self.assertEqual(info.class_name, "GameObject")

    def test_classify_gd_vector_rejects_non_object_elements(self) -> None:
        self.assertIsNone(classify_arg("gd::vector<int>", {}))

    def test_classify_gd_map_stays_unsupported(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        objects = {"CCObject": ccobject, "cocos2d::CCObject": ccobject}

        self.assertIsNone(classify_arg("gd::map<int, cocos2d::CCObject*>", objects))


class FmodTypeMapTests(unittest.TestCase):
    def test_classify_fmod_enums(self) -> None:
        for name in ("FMOD_RESULT", "FMOD_OPENSTATE", "FMOD_SPEAKERMODE"):
            info = classify_arg(name, {})
            self.assertIsNotNone(info)
            assert info is not None
            self.assertEqual(info.kind, "enum")
            self.assertEqual(info.lua_type, "number")
            self.assertEqual(info.cxx_type, name)

    def test_classify_fmod_opaque_handles(self) -> None:
        cases = {
            "FMOD::Channel*": "FMODChannel",
            "FMOD::Sound*": "FMODSound",
            "FMOD::ChannelGroup*": "FMODChannelGroup",
            "FMODSound*": "FMODSound",
        }
        for cxx, lua in cases.items():
            info = classify_arg(cxx, {})
            self.assertIsNotNone(info)
            assert info is not None
            self.assertEqual(info.kind, "opaque_handle")
            self.assertEqual(info.cxx_type, cxx)
            self.assertEqual(info.lua_type, lua)

    def test_classify_fmod_channel_return_is_optional(self) -> None:
        info = classify_return("FMOD::Channel*", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "opaque_handle")
        self.assertEqual(info.lua_type, "FMODChannel?")

    def test_classify_fmod_sound_alias_return_is_optional(self) -> None:
        info = classify_return("FMODSound*", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "opaque_handle")
        self.assertEqual(info.cxx_type, "FMODSound*")
        self.assertEqual(info.lua_type, "FMODSound?")

    def test_classify_unlisted_fmod_pointer_stays_unsupported(self) -> None:
        self.assertIsNone(classify_arg("FMOD::DSP*", {}))
