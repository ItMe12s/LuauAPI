from __future__ import annotations

import unittest
from helpers import (
    Arg,  # type: ignore[import-unresolved]
    COCOS_ENUM_TYPES,  # type: ignore[import-unresolved]
    Class,  # type: ignore[import-unresolved]
    GD_ENUM_TYPES,  # type: ignore[import-unresolved]
    Method,  # type: ignore[import-unresolved]
    Root,  # type: ignore[import-unresolved]
    _input_arg_count,  # type: ignore[import-unresolved]
    classify_arg,  # type: ignore[import-unresolved]
    classify_return,  # type: ignore[import-unresolved]
    codegen_object_map,  # type: ignore[import-unresolved]
    object_classes,  # type: ignore[import-unresolved]
    register_geode_enums,  # type: ignore[import-unresolved]
)


class GeodeEnumRegistrationTests(unittest.TestCase):
    def test_register_geode_enums_skips_reserved(self) -> None:
        skip = GD_ENUM_TYPES | COCOS_ENUM_TYPES | {"CollidingClass"}
        ctx = register_geode_enums(
            {
                "IconType": "IconType",
                "enumKeyCodes": "cocos2d::enumKeyCodes",
                "CollidingClass": "geode::CollidingClass",
                "UniqueGeodeEnum": "geode::UniqueGeodeEnum",
            },
            skip=skip,
        )
        self.assertNotIn("IconType", ctx.geode_enum_names)
        self.assertNotIn("enumKeyCodes", ctx.geode_enum_names)
        self.assertNotIn("CollidingClass", ctx.geode_enum_names)
        self.assertIn("UniqueGeodeEnum", ctx.geode_enum_names)

    def test_register_geode_enums_keeps_unique(self) -> None:
        ctx = register_geode_enums({"BaseType": "geode::BaseType"})
        self.assertEqual(ctx.geode_enum_names, frozenset({"BaseType"}))
        self.assertIn("BaseType", ctx.enum_cxx_names())

    def test_classify_arg_no_enum_shadow(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        colliding = Class(name="CollidingClass", namespace="geode", bases=["CCObject"])
        root = Root(classes=[ccobject, colliding])
        objects = codegen_object_map(root)
        skip = GD_ENUM_TYPES | COCOS_ENUM_TYPES | {c.name for c in object_classes(root)}
        ctx = register_geode_enums(
            {"CollidingClass": "geode::CollidingClass"}, skip=skip
        )
        info = classify_arg("CollidingClass*", objects, ctx=ctx)
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

    def test_classify_gd_vector_int_as_primitive_vector(self) -> None:
        info = classify_arg("gd::vector<int>", {})

        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "primitive_vector")
        self.assertEqual(info.cxx_type, "gd::vector<int>")
        self.assertEqual(info.lua_type, "{ number }")
        self.assertIsNotNone(info.element_type)
        assert info.element_type is not None
        self.assertEqual(info.element_type.kind, "number")
        self.assertFalse(info.is_ref)
        self.assertFalse(info.is_out)

    def test_classify_gd_vector_int_const_ref(self) -> None:
        info = classify_arg("gd::vector<int> const&", {})

        assert info is not None
        self.assertEqual(info.kind, "primitive_vector")
        self.assertTrue(info.is_ref)
        self.assertFalse(info.is_out)

    def test_classify_gd_vector_bool_as_primitive_vector(self) -> None:
        info = classify_arg("gd::vector<bool>", {})

        assert info is not None
        self.assertEqual(info.kind, "primitive_vector")
        self.assertEqual(info.lua_type, "{ boolean }")
        assert info.element_type is not None
        self.assertEqual(info.element_type.kind, "bool")

    def test_classify_gd_vector_string_as_primitive_vector(self) -> None:
        info = classify_arg("gd::vector<std::string>", {})

        assert info is not None
        self.assertEqual(info.kind, "primitive_vector")
        self.assertEqual(info.lua_type, "{ string }")
        assert info.element_type is not None
        self.assertEqual(info.element_type.kind, "string")

    def test_classify_gd_vector_wideint_as_primitive_vector(self) -> None:
        info = classify_arg("gd::vector<uint64_t>", {})

        assert info is not None
        self.assertEqual(info.kind, "primitive_vector")
        self.assertEqual(info.lua_type, "{ string }")
        assert info.element_type is not None
        self.assertEqual(info.element_type.kind, "wideint")

    def test_classify_gd_vector_enum_as_primitive_vector(self) -> None:
        info = classify_arg("gd::vector<FMOD_RESULT>", {})

        assert info is not None
        self.assertEqual(info.kind, "primitive_vector")
        self.assertEqual(info.lua_type, "{ number }")
        assert info.element_type is not None
        self.assertEqual(info.element_type.kind, "enum")
        self.assertEqual(info.element_type.cxx_type, "FMOD_RESULT")

    def test_classify_gd_vector_object_pointer_stays_vector_view(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        objects = {"CCObject": ccobject, "cocos2d::CCObject": ccobject}

        info = classify_arg("gd::vector<cocos2d::CCObject*>", objects)

        assert info is not None
        self.assertEqual(info.kind, "vector_view")
        self.assertEqual(info.lua_type, "{ CCObject? }")

    def test_classify_gd_vector_rejects_nested_containers(self) -> None:
        self.assertIsNone(classify_arg("gd::vector<gd::vector<int>>", {}))

    def test_classify_gd_map_int_object_pointer(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        objects = {"CCObject": ccobject, "cocos2d::CCObject": ccobject}

        info = classify_arg("gd::map<int, cocos2d::CCObject*>", objects)

        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "map")
        self.assertEqual(info.cxx_type, "gd::map<int, cocos2d::CCObject*>")
        self.assertEqual(info.lua_type, "{ [number]: CCObject? }")
        assert info.key_type is not None
        assert info.value_type is not None
        self.assertEqual(info.key_type.kind, "number")
        self.assertEqual(info.value_type.kind, "object")
        self.assertEqual(info.value_type.class_name, "CCObject")
        self.assertFalse(info.is_ref)
        self.assertFalse(info.is_out)

    def test_classify_gd_map_primitive_value_pair(self) -> None:
        info = classify_arg("gd::map<int, bool>", {})

        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "map")
        self.assertEqual(info.lua_type, "{ [number]: boolean }")
        assert info.key_type is not None
        assert info.value_type is not None
        self.assertEqual(info.key_type.kind, "number")
        self.assertEqual(info.value_type.kind, "bool")

    def test_classify_gd_map_string_key(self) -> None:
        info = classify_arg("gd::map<std::string, int>", {})

        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "map")
        self.assertEqual(info.lua_type, "{ [string]: number }")
        assert info.key_type is not None
        self.assertEqual(info.key_type.kind, "string")

    def test_classify_gd_map_const_ref(self) -> None:
        info = classify_arg("gd::map<int, int> const&", {})

        assert info is not None
        self.assertEqual(info.kind, "map")
        self.assertTrue(info.is_ref)
        self.assertFalse(info.is_out)

    def test_classify_gd_unordered_map_primitive_pair(self) -> None:
        info = classify_arg("gd::unordered_map<int, int>", {})

        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "unordered_map")
        self.assertEqual(info.cxx_type, "gd::unordered_map<int, int>")
        self.assertEqual(info.lua_type, "{ [number]: number }")
        assert info.key_type is not None
        assert info.value_type is not None
        self.assertEqual(info.key_type.kind, "number")
        self.assertEqual(info.value_type.kind, "number")

    def test_classify_gd_set_int(self) -> None:
        info = classify_arg("gd::set<int>", {})

        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "set")
        self.assertEqual(info.cxx_type, "gd::set<int>")
        self.assertEqual(info.lua_type, "{ number }")
        assert info.element_type is not None
        self.assertEqual(info.element_type.kind, "number")
        self.assertFalse(info.is_ref)
        self.assertFalse(info.is_out)

    def test_classify_gd_set_string(self) -> None:
        info = classify_arg("gd::set<std::string>", {})

        assert info is not None
        self.assertEqual(info.kind, "set")
        self.assertEqual(info.lua_type, "{ string }")
        assert info.element_type is not None
        self.assertEqual(info.element_type.kind, "string")

    def test_classify_gd_unordered_set_int(self) -> None:
        info = classify_arg("gd::unordered_set<int>", {})

        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "unordered_set")
        self.assertEqual(info.cxx_type, "gd::unordered_set<int>")
        self.assertEqual(info.lua_type, "{ number }")
        assert info.element_type is not None
        self.assertEqual(info.element_type.kind, "number")

    def test_classify_gd_map_rejects_object_keys(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        objects = {"CCObject": ccobject, "cocos2d::CCObject": ccobject}

        self.assertIsNone(classify_arg("gd::map<cocos2d::CCObject*, int>", objects))

    def test_classify_gd_map_rejects_nested_containers(self) -> None:
        self.assertIsNone(classify_arg("gd::map<int, gd::vector<int>>", {}))

    def test_classify_gd_set_rejects_nested_containers(self) -> None:
        self.assertIsNone(classify_arg("gd::set<gd::vector<int>>", {}))


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
