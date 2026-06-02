from __future__ import annotations

import unittest
from helpers import (
    Class,  # type: ignore[import-unresolved]
    Root,  # type: ignore[import-unresolved]
    build_class_lookup,  # type: ignore[import-unresolved]
    codegen_object_map,  # type: ignore[import-unresolved]
    resolve_base,  # type: ignore[import-unresolved]
)


class F4ClassLookupCollisionTests(unittest.TestCase):
    def test_ambiguous_short_name_resolved_by_qualified(self) -> None:
        cls_a = Class(name="Sprite", namespace="ns1", bases=["cocos2d::CCObject"])
        cls_b = Class(name="Sprite", namespace="ns2", bases=["cocos2d::CCObject"])
        lookup = build_class_lookup([cls_a, cls_b])
        self.assertIs(lookup["ns1::Sprite"], cls_a)
        self.assertIs(lookup["ns2::Sprite"], cls_b)
        self.assertNotIn("Sprite", lookup)

    def test_unambiguous_short_name_works(self) -> None:
        cls = Class(name="CCObject", namespace="cocos2d")
        lookup = build_class_lookup([cls])
        self.assertIs(lookup["CCObject"], cls)
        self.assertIs(lookup["cocos2d::CCObject"], cls)

    def test_resolve_base_tries_qualified_then_short(self) -> None:
        cls = Class(name="CCNode", namespace="cocos2d")
        lookup = build_class_lookup([cls])
        self.assertIs(resolve_base(lookup, "cocos2d::CCNode"), cls)
        self.assertIs(resolve_base(lookup, "CCNode"), cls)
        self.assertIsNone(resolve_base(lookup, "Unknown"))

    def test_codegen_object_map_drops_ambiguous_short_name(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls_a = Class(name="Sprite", namespace="ns1", bases=["cocos2d::CCObject"])
        cls_b = Class(name="Sprite", namespace="ns2", bases=["cocos2d::CCObject"])
        objects = codegen_object_map(Root(classes=[ccobject, cls_a, cls_b]))
        self.assertIs(objects["ns1::Sprite"], cls_a)
        self.assertIs(objects["ns2::Sprite"], cls_b)
        self.assertNotIn("Sprite", objects)
