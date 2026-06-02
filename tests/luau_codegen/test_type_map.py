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

    def test_classify_gd_vector_rejects_non_object_elements(self) -> None:
        self.assertIsNone(classify_arg("gd::vector<int>", {}))

    def test_classify_gd_map_stays_unsupported(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        objects = {"CCObject": ccobject, "cocos2d::CCObject": ccobject}

        self.assertIsNone(classify_arg("gd::map<int, cocos2d::CCObject*>", objects))
