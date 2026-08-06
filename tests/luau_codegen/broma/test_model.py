from __future__ import annotations

import unittest

import test_support  # noqa: F401 - adds tools to sys.path
from luau_codegen.model.domain import (  # type: ignore[import-unresolved]
    ClassHierarchy,
    build_class_lookup,
    codegen_object_map,
    object_classes,
    resolve_base,
)
from luau_codegen.parse.broma import Class, Root  # type: ignore[import-unresolved]


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

    def test_hierarchy_uses_qualified_base_when_short_name_is_ambiguous(self) -> None:
        root = Class(name="CCObject", namespace="cocos2d")
        valid = Class(name="Sprite", namespace="ns1", bases=["cocos2d::CCObject"])
        unrelated = Class(name="Sprite", namespace="ns2")
        qualified = Class(name="Qualified", bases=["ns1::Sprite"])
        ambiguous = Class(name="Ambiguous", bases=["Sprite"])
        hierarchy = ClassHierarchy([root, valid, unrelated, qualified, ambiguous])
        self.assertTrue(hierarchy.is_ccobject_descendant(qualified))
        self.assertFalse(hierarchy.is_ccobject_descendant(ambiguous))

    def test_hierarchy_handles_diamonds_and_cycles(self) -> None:
        root = Class(name="CCObject", namespace="cocos2d")
        left = Class(name="Left", bases=["CCObject"])
        right = Class(name="Right", bases=["CCObject"])
        leaf = Class(name="Leaf", bases=["Left", "Right"])
        self_cycle = Class(name="SelfCycle", bases=["SelfCycle"])
        cycle_a = Class(name="CycleA", bases=["CycleB"])
        cycle_b = Class(name="CycleB", bases=["CycleA"])
        hierarchy = ClassHierarchy([root, left, right, leaf, self_cycle, cycle_a, cycle_b])
        self.assertEqual(hierarchy.depth(leaf), 2)
        self.assertFalse(hierarchy.is_ccobject_descendant(self_cycle))
        self.assertFalse(hierarchy.is_ccobject_descendant(cycle_a))
        self.assertFalse(hierarchy.is_ccobject_descendant(cycle_b))

    def test_cycle_branch_can_reach_ccobject_through_another_base(self) -> None:
        root = Class(name="CCObject", namespace="cocos2d")
        cycle_b = Class(name="CycleB", bases=["CycleA"])
        cycle_a = Class(name="CycleA", bases=["CycleB", "CCObject"])
        classes = object_classes(Root(classes=[cycle_b, cycle_a, root]))
        self.assertEqual([cls.name for cls in classes], ["CCObject", "CycleA", "CycleB"])
