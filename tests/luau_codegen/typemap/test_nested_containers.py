from __future__ import annotations

import os
import re
import unittest

from test_support import ROOT, all_platforms, types_text
from luau_codegen.convert.marshalling import check_arg, push_value  # type: ignore[import-unresolved]
from luau_codegen.convert.type_map import classify_arg  # type: ignore[import-unresolved]
from luau_codegen.emit.luau_types import emit_luau_types  # type: ignore[import-unresolved]
from luau_codegen.parse.broma import (  # type: ignore[import-unresolved]
    Arg,
    Class,
    Field,
    Method,
    Root,
)
from luau_codegen.policy.filtering import group_supported  # type: ignore[import-unresolved]
from luau_codegen.policy.fields import bindable_field  # type: ignore[import-unresolved]


class RecursiveContainerTypeMapTests(unittest.TestCase):
    def test_supported_composites_recurse(self) -> None:
        cases = (
            (
                "gd::map<int, gd::vector<int>>",
                "map",
                "{ [number]: { number } }",
            ),
            (
                "gd::unordered_map<int, gd::set<gd::string>>",
                "unordered_map",
                "{ [number]: { string } }",
            ),
            (
                "gd::vector<gd::unordered_map<int, int>>",
                "vector",
                "{ { [number]: number } }",
            ),
            (
                "gd::set<std::pair<int, gd::vector<bool>>>",
                "set",
                "{ { first: number, second: { boolean } } }",
            ),
            (
                "gd::unordered_set<std::tuple<int, bool>>",
                "unordered_set",
                "{ { (number | boolean) } }",
            ),
            (
                "std::array<gd::vector<int>, 3>",
                "std_array",
                "{ { number } }",
            ),
        )
        for cxx, kind, lua in cases:
            with self.subTest(cxx=cxx):
                info = classify_arg(cxx, {})
                self.assertIsNotNone(info)
                assert info is not None
                self.assertEqual(info.kind, kind)
                self.assertEqual(info.lua_type, lua)

    def test_tuple_shapes_and_positions(self) -> None:
        cases = (
            ("std::tuple<>", "{}", ()),
            ("std::tuple<int, int, int, int>", "{ number }", ("number",) * 4),
            (
                "std::tuple<int, bool, gd::string, gd::vector<float>>",
                "{ (number | boolean | string | { number }) }",
                ("number", "boolean", "string", "{ number }"),
            ),
        )
        for cxx, lua, children in cases:
            with self.subTest(cxx=cxx):
                info = classify_arg(cxx, {})
                self.assertIsNotNone(info)
                assert info is not None
                self.assertEqual(info.kind, "tuple")
                self.assertEqual(info.lua_type, lua)
                self.assertEqual(tuple(t.lua_type for t in info.tuple_types), children)

    def test_nesting_has_no_codegen_depth_cap(self) -> None:
        cxx = "int"
        for _ in range(32):
            cxx = f"gd::vector<{cxx}>"
        info = classify_arg(cxx, {})
        self.assertIsNotNone(info)
        assert info is not None
        depth = 0
        node = info
        while node.element_type is not None:
            depth += 1
            node = node.element_type
        self.assertEqual(depth, 32)
        self.assertEqual(node.kind, "number")

    def test_recursive_root_pointer_keeps_out_flags(self) -> None:
        info = classify_arg("gd::map<int, gd::vector<int>>*", {})
        self.assertIsNotNone(info)
        assert info is not None and info.value_type is not None
        self.assertEqual(info.kind, "map")
        self.assertEqual(info.value_type.kind, "vector")
        self.assertTrue(info.is_out)
        self.assertTrue(info.is_vector_ptr)

    def test_invalid_descendants_are_rejected(self) -> None:
        rejected = (
            "gd::map<gd::vector<int>, int>",
            "gd::map<std::tuple<int, int>, int>",
            "gd::map<std::pair<int, gd::vector<int>>, int>",
            "gd::map<gd::vector<int>*, int>",
            "gd::vector<gd::vector<int>*>",
            "gd::vector<gd::vector<int>**>",
            "gd::vector<int>**",
            "gd::map<int, gd::vector<int>*>",
            "gd::unordered_map<int, gd::vector<int>*>",
            "gd::set<gd::vector<int>*>",
            "gd::unordered_set<gd::vector<int>*>",
            "std::array<gd::vector<int>*, 2>",
            "std::pair<int, gd::vector<int>*>",
            "std::tuple<int, gd::vector<int>*>",
            "gd::vector<std::vector<int>>",
            "gd::vector<ccCArray<int>>",
            "gd::vector<cocos2d::ccCArray*>",
            "gd::vector<std::function<void()>>",
            "gd::vector<arc::TaskHandle<int>>",
            "gd::vector<geode::Result<int>>",
            "gd::vector<char*>",
            "gd::vector<std::string_view>",
            "std::array<int, 0>",
            "std::array<int, 2001>",
        )
        for cxx in rejected:
            with self.subTest(cxx=cxx):
                self.assertIsNone(classify_arg(cxx, {}))


class RecursiveContainerMarshallingTests(unittest.TestCase):
    def test_runtime_has_no_exception_only_container_path(self) -> None:
        path = os.path.join(ROOT, "src", "framework", "stack", "ContainerTables.hpp")
        with open(path, encoding="utf-8") as f:
            text = f.read()
        self.assertIsNone(re.search(r"\btry\s*\{|\bcatch\s*\(|\bthrow\b", text))
        self.assertNotIn("LUA_USE_LONGJMP", text)

    def test_recursive_composites_use_generic_runtime_entry_points(self) -> None:
        cases = (
            "gd::map<int, gd::vector<int>>",
            "std::tuple<int, bool, float>",
        )
        for cxx in cases:
            with self.subTest(cxx=cxx):
                info = classify_arg(cxx, {})
                self.assertIsNotNone(info)
                assert info is not None
                check_text = "".join(check_arg(Arg(cxx, "value"), info, 1, "arg0", "test"))
                push_text = "".join(push_value(info, "value"))
                self.assertIn(f"checkContainerValue<{info.cxx_type}>", check_text)
                self.assertIn(f"pushContainerValue<{info.cxx_type}>", push_text)

    def test_by_value_tree_uses_generic_helper(self) -> None:
        info = classify_arg("gd::vector<gd::vector<int>>", {})
        self.assertIsNotNone(info)
        assert info is not None
        text = "".join(push_value(info, "sizes"))
        self.assertIn("pushContainerValue<", text)


class RecursiveContainerSurfacePolicyTests(unittest.TestCase):
    def test_value_tree_methods_bind(self) -> None:
        foo = Class(
            name="Foo",
            methods=[
                Method(
                    name="roundTrip",
                    ret="gd::map<int, gd::vector<int>>",
                    args=[Arg("gd::map<int, gd::vector<int>>", "value")],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        grouped, skipped = group_supported(foo, {"Foo": foo}, "win")
        self.assertEqual(skipped, [])
        self.assertIn("roundTrip", grouped)

    def test_composite_pointer_descendant_method_is_rejected(self) -> None:
        foo = Class(
            name="Foo",
            methods=[
                Method(
                    name="mutate",
                    ret="void",
                    args=[Arg("gd::vector<gd::vector<int>*>", "value")],
                    platforms=all_platforms("0x1"),
                ),
                Method(
                    name="values",
                    ret="gd::vector<gd::vector<int>*>",
                    args=[],
                    platforms=all_platforms("0x2"),
                ),
                Method(
                    name="fill",
                    ret="void",
                    args=[Arg("gd::vector<gd::vector<int>*>&", "value")],
                    platforms=all_platforms("0x3"),
                ),
            ],
        )
        grouped, skipped = group_supported(foo, {"Foo": foo}, "win")
        self.assertNotIn("mutate", grouped)
        self.assertNotIn("values", grouped)
        self.assertNotIn("fill", grouped)
        self.assertEqual(len(skipped), 3)
        self.assertTrue(any("unsupported" in item[-1] for item in skipped))

    def test_non_audited_composite_pointer_field_is_rejected(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        foo = Class(
            name="Foo",
            bases=["CCObject"],
            fields=[Field("m_sectionSizes", "gd::vector<gd::vector<int>*>")],
        )
        objects = {
            "CCObject": ccobject,
            "cocos2d::CCObject": ccobject,
            "Foo": foo,
        }
        ok, reason, _, _ = bindable_field(foo.fields[0], objects, foo)
        self.assertFalse(ok)
        self.assertIn("unsupported", reason)

    def test_deep_inaccessible_object_leaf_rejects_field(self) -> None:
        ccgrabber = Class(name="CCGrabber", namespace="cocos2d")
        foo = Class(name="Foo")
        field = Field(
            "m_values",
            "gd::map<int, gd::vector<cocos2d::CCGrabber*>>",
        )
        ok, reason, _, _ = bindable_field(
            field,
            {"CCGrabber": ccgrabber, "cocos2d::CCGrabber": ccgrabber},
            foo,
        )
        self.assertFalse(ok)
        self.assertEqual(reason, "inaccessible-type:CCGrabber")


class RecursiveContainerStubTests(unittest.TestCase):
    def test_container_null_sentinel_is_not_declared(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        text = types_text(emit_luau_types(Root(classes=[ccobject])))
        self.assertNotIn("ContainerNull", text)
        self.assertNotIn("nullContainer", text)

    def test_deep_object_reference_is_declared_before_use(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        leaf = Class(name="Leaf", bases=["CCObject"])
        foo = Class(
            name="Foo",
            bases=["CCObject"],
            methods=[
                Method(
                    name="leaves",
                    ret="gd::map<int, gd::vector<Leaf*>>",
                    args=[],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        text = types_text(emit_luau_types(Root(classes=[ccobject, foo, leaf])))
        leaf_pos = text.index("declare class Leaf")
        method_pos = text.index("function leaves(self)")
        self.assertLess(leaf_pos, method_pos)


if __name__ == "__main__":
    unittest.main()
