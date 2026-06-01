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
