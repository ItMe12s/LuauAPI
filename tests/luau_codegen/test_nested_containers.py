from __future__ import annotations

import unittest

from luau_codegen.convert.marshalling import check_arg, push_value  # type: ignore[import-unresolved]
from luau_codegen.convert.type_map import TypeInfo, classify_arg  # type: ignore[import-unresolved]
from luau_codegen.model.nested_containers import (  # type: ignore[import-unresolved]
    BASELINE_NESTED_MAP_FIELDS,
    BASELINE_NESTED_PRIMITIVE_VECTOR_FIELDS,
    BASELINE_NESTED_UNORDERED_MAP_FIELDS,
    allow_nested_map_value,
    allow_nested_primitive_vector_outer,
)
from luau_codegen.model.pair_design import pair_key_map_entry_lua_type  # type: ignore[import-unresolved]
from luau_codegen.parse.broma import Arg, Class, Field  # type: ignore[import-unresolved]
from luau_codegen.emit.bindings.class_file import _emit_class_file  # type: ignore[import-unresolved]
from luau_codegen.policy.filtering import group_supported  # type: ignore[import-unresolved]


class NestedContainerPolicyTests(unittest.TestCase):
    def test_allowlist_maps_documented(self) -> None:
        self.assertIn("m_unkMap770", BASELINE_NESTED_MAP_FIELDS)
        self.assertIn("m_labelObjects", BASELINE_NESTED_UNORDERED_MAP_FIELDS)
        self.assertIn("m_sectionSizes", BASELINE_NESTED_PRIMITIVE_VECTOR_FIELDS)

    def test_primitive_nested_vector_outer_shape(self) -> None:
        self.assertTrue(
            allow_nested_primitive_vector_outer("gd::vector<gd::vector<int>*>")
        )
        self.assertFalse(
            allow_nested_primitive_vector_outer("gd::vector<gd::vector<bool>*>")
        )


class NestedContainerTypeMapTests(unittest.TestCase):
    def test_unordered_map_int_to_label_object_vector(self) -> None:
        label = Class(name="LabelGameObject", namespace="")
        objects = {"LabelGameObject": label}

        info = classify_arg(
            "gd::unordered_map<int, gd::vector<LabelGameObject*>>", objects
        )

        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "unordered_map")
        assert info.value_type is not None
        self.assertEqual(info.value_type.kind, "vector_view")
        self.assertEqual(info.value_type.lua_type, "{ LabelGameObject? }")
        self.assertEqual(info.lua_type, "{ [number]: { LabelGameObject? } }")

    def test_pair_key_map_to_opaque_object_vector_entry_list(self) -> None:
        info = classify_arg(
            "gd::map<std::pair<int, int>, gd::vector<GroupCommandObject2*>>", {}
        )

        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "map")
        assert info.value_type is not None
        self.assertEqual(info.value_type.kind, "vector_view")
        self.assertIn("GroupCommandObject2", info.lua_type)
        self.assertIn("value:", info.lua_type)
        self.assertNotIn("[", info.lua_type)

    def test_map_int_to_primitive_vector_still_rejected(self) -> None:
        self.assertIsNone(classify_arg("gd::map<int, gd::vector<int>>", {}))

    def test_nested_primitive_vector_int_pointers(self) -> None:
        info = classify_arg("gd::vector<gd::vector<int>*>", {})

        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "nested_primitive_vector_view")
        self.assertEqual(info.lua_type, "{ { number } }")

    def test_deep_nested_vector_still_rejected(self) -> None:
        self.assertIsNone(classify_arg("gd::vector<gd::vector<gd::vector<int>*>>", {}))


class NestedContainerMarshallingTests(unittest.TestCase):
    def _map_with_vector_value(
        self, *, kind: str, cxx: str, lua: str, value: TypeInfo, key: TypeInfo
    ) -> TypeInfo:
        return TypeInfo(
            kind=kind,
            cxx_type=cxx,
            lua_type=lua,
            key_type=key,
            value_type=value,
        )

    def test_unordered_map_nested_vector_marshalling(self) -> None:
        key = TypeInfo(kind="number", cxx_type="int", lua_type="number")
        value = TypeInfo(
            kind="vector_view",
            cxx_type="gd::vector<LabelGameObject*>",
            lua_type="{ LabelGameObject? }",
            element_type=TypeInfo(
                kind="object",
                cxx_type="LabelGameObject*",
                lua_type="LabelGameObject",
                class_name="LabelGameObject",
            ),
        )
        info = self._map_with_vector_value(
            kind="unordered_map",
            cxx="gd::unordered_map<int, gd::vector<LabelGameObject*>>",
            lua="{ [number]: { LabelGameObject? } }",
            value=value,
            key=key,
        )

        check_text = "".join(
            check_arg(
                Arg("gd::unordered_map<int, gd::vector<LabelGameObject*>>", "labels"),
                info,
                1,
                "arg0",
                "test",
            )
        )
        push_text = "".join(push_value(info, "labels"))

        self.assertIn(
            "luax::checkUnorderedMap<int, gd::vector<LabelGameObject*>>", check_text
        )
        self.assertIn(
            "luax::pushUnorderedMap<int, gd::vector<LabelGameObject*>>", push_text
        )

    def test_pair_key_map_nested_opaque_vector_marshalling(self) -> None:
        key = TypeInfo(
            kind="pair",
            cxx_type="std::pair<int, int>",
            lua_type="{ first: number, second: number }",
            key_type=TypeInfo(kind="number", cxx_type="int", lua_type="number"),
            value_type=TypeInfo(kind="number", cxx_type="int", lua_type="number"),
        )
        value = TypeInfo(
            kind="vector_view",
            cxx_type="gd::vector<GroupCommandObject2*>",
            lua_type="{ GroupCommandObject2? }",
            element_type=TypeInfo(
                kind="opaque_handle",
                cxx_type="GroupCommandObject2*",
                lua_type="GroupCommandObject2",
            ),
        )
        info = self._map_with_vector_value(
            kind="map",
            cxx="gd::map<std::pair<int, int>, gd::vector<GroupCommandObject2*>>",
            lua=pair_key_map_entry_lua_type(
                "number", "number", "{ GroupCommandObject2? }"
            ),
            value=value,
            key=key,
        )

        check_text = "".join(
            check_arg(
                Arg(
                    "gd::map<std::pair<int, int>, gd::vector<GroupCommandObject2*>>",
                    "groups",
                ),
                info,
                1,
                "arg0",
                "test",
            )
        )
        push_text = "".join(push_value(info, "groups"))

        self.assertIn(
            "luax::checkPairKeyMap<int, int, gd::vector<GroupCommandObject2*>>",
            check_text,
        )
        self.assertIn(
            "luax::pushPairKeyMap<int, int, gd::vector<GroupCommandObject2*>>",
            push_text,
        )

    def test_nested_primitive_vector_push_helper(self) -> None:
        inner = TypeInfo(
            kind="primitive_vector",
            cxx_type="gd::vector<int>",
            lua_type="{ number }",
            element_type=TypeInfo(kind="number", cxx_type="int", lua_type="number"),
        )
        info = TypeInfo(
            kind="nested_primitive_vector_view",
            cxx_type="gd::vector<gd::vector<int>*>",
            lua_type="{ { number } }",
            element_type=inner,
        )

        text = "".join(push_value(info, "sizes"))

        self.assertIn("luax::pushNestedPrimitiveVectorPointers<int>", text)


class NestedContainerFieldBindingTests(unittest.TestCase):
    def test_nested_primitive_vector_field_is_readonly(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        foo = Class(
            name="Foo",
            namespace="cocos2d",
            bases=["CCObject"],
            fields=[Field("m_sectionSizes", "gd::vector<gd::vector<int>*>")],
        )
        grouped, _ = group_supported(foo, {}, "win")
        text = _emit_class_file(
            foo,
            grouped,
            [],
            [(foo, foo.fields[0])],
            {"CCObject": ccobject, "cocos2d::CCObject": ccobject},
            set(),
            1,
            "win",
        )

        self.assertIn("pushNestedPrimitiveVectorPointers<int>", text)
        self.assertIn("readonlyField", text)
        self.assertNotIn("field_set_", text)

    def test_nested_map_field_setter_uses_assign(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        label = Class(name="LabelGameObject", namespace="")
        foo = Class(
            name="Foo",
            namespace="cocos2d",
            bases=["CCObject"],
            fields=[
                Field(
                    "m_labelObjects",
                    "gd::unordered_map<int, gd::vector<LabelGameObject*>>",
                )
            ],
        )
        objects = {
            "CCObject": ccobject,
            "cocos2d::CCObject": ccobject,
            "LabelGameObject": label,
        }
        grouped, _ = group_supported(foo, objects, "win")
        text = _emit_class_file(
            foo,
            grouped,
            [],
            [(foo, foo.fields[0])],
            objects,
            set(),
            1,
            "win",
        )

        self.assertIn("checkUnorderedMap<int, gd::vector<LabelGameObject*>>", text)
        self.assertIn(
            "luax::assignUnorderedMap(*self->m_labelObjects, std::move(value))", text
        )


class NestedMapValuePolicyUnitTests(unittest.TestCase):
    def test_allow_nested_map_value_object_vector(self) -> None:
        key = TypeInfo(kind="number", cxx_type="int", lua_type="number")
        value = TypeInfo(
            kind="vector_view",
            cxx_type="gd::vector<LabelGameObject*>",
            lua_type="{ LabelGameObject? }",
            element_type=TypeInfo(
                kind="object",
                cxx_type="LabelGameObject*",
                lua_type="LabelGameObject",
                class_name="LabelGameObject",
            ),
        )
        self.assertTrue(allow_nested_map_value(key, value))

    def test_reject_primitive_vector_map_value(self) -> None:
        key = TypeInfo(kind="number", cxx_type="int", lua_type="number")
        value = TypeInfo(
            kind="primitive_vector",
            cxx_type="gd::vector<int>",
            lua_type="{ number }",
            element_type=TypeInfo(kind="number", cxx_type="int", lua_type="number"),
        )
        self.assertFalse(allow_nested_map_value(key, value))


if __name__ == "__main__":
    unittest.main()
