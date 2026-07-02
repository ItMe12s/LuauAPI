from __future__ import annotations

import unittest
import warnings

from test_support import resolve_test_bindings_dir
from luau_codegen.convert.marshalling import check_arg, push_value  # type: ignore[import-unresolved]
from luau_codegen.convert.type_map import TypeInfo, classify_arg  # type: ignore[import-unresolved]
from luau_codegen.model.domain import build_class_lookup  # type: ignore[import-unresolved]
from luau_codegen.model.nested_containers import (  # type: ignore[import-unresolved]
    BASELINE_MAP_VECTOR_FIELDS,
    BASELINE_NESTED_BOOL_VECTOR_FIELDS,
    BASELINE_NESTED_OBJECT_GRID_FIELDS,
    BASELINE_NESTED_OBJECT_VECTOR_FIELDS,
    BASELINE_TUPLE_SET_FIELDS,
)
from luau_codegen.parse.broma import Arg, Class, Field  # type: ignore[import-unresolved]
from luau_codegen.parse.collect import collect_bindings_root  # type: ignore[import-unresolved]
from luau_codegen.policy.fields import bindable_field  # type: ignore[import-unresolved]
from luau_codegen.emit.bindings.class_file import _emit_class_file  # type: ignore[import-unresolved]
from luau_codegen.policy.filtering import group_supported  # type: ignore[import-unresolved]


_GJ_BINDABLE_FIELDS = (
    "m_varianceValues",
    "m_nonEffectObjectsFlags",
    "m_collisionBlockSections",
    "m_sections",
    "m_nonEffectObjects",
    "m_spawnRemapTriggers",
    "m_spawnTuples",
)

_GJ_STILL_SKIPPED = (
    "m_unk1000",
    "m_unk8a0",
    "m_unk3340",
    "m_unk3358",
)


class GjGridFieldTypeMapTests(unittest.TestCase):
    def test_variance_array_classifies(self) -> None:
        info = classify_arg("std::array<float, 2000>", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "std_array")
        self.assertEqual(info.array_size, 2000)

    def test_nested_bool_grid_classifies(self) -> None:
        info = classify_arg("gd::vector<gd::vector<bool>*>", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "nested_bool_vector_view")
        self.assertEqual(info.lua_type, "{ { boolean } }")

    def test_nested_object_vector_classifies(self) -> None:
        game_object = Class(name="GameObject", namespace="")
        info = classify_arg("gd::vector<gd::vector<GameObject*>*>", {"GameObject": game_object})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "nested_object_vector_view")
        self.assertEqual(info.lua_type, "{ { GameObject? } }")

    def test_nested_object_grid_classifies(self) -> None:
        game_object = Class(name="GameObject", namespace="")
        info = classify_arg(
            "gd::vector<gd::vector<gd::vector<GameObject*>*>*>",
            {"GameObject": game_object},
        )
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "nested_object_grid_view")
        self.assertEqual(info.lua_type, "{ { { GameObject? } } }")

    def test_map_vector_classifies(self) -> None:
        info = classify_arg("gd::vector<gd::unordered_map<int,int>>", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "map_vector")
        self.assertEqual(info.lua_type, "{ { [number]: number } }")

    def test_tuple_set_classifies(self) -> None:
        info = classify_arg("gd::set<std::tuple<int, int, int>>", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "set")
        self.assertEqual(info.lua_type, "{ { number, number, number } }")


class GjGridFieldMarshallingTests(unittest.TestCase):
    def test_map_vector_marshalling(self) -> None:
        map_info = classify_arg("gd::unordered_map<int,int>", {})
        assert map_info is not None
        info = TypeInfo(
            kind="map_vector",
            cxx_type="gd::vector<gd::unordered_map<int,int>>",
            lua_type="{ { [number]: number } }",
            element_type=map_info,
        )
        check_text = "".join(
            check_arg(
                Arg("gd::vector<gd::unordered_map<int,int>>", "remaps"),
                info,
                1,
                "arg0",
                "test",
            )
        )
        push_text = "".join(push_value(info, "remaps"))
        self.assertIn("checkPrimitiveVector<gd::unordered_map<int, int>>", check_text)
        self.assertIn("pushPrimitiveVector<gd::unordered_map<int, int>>", push_text)

    def test_nested_object_grid_push_uses_owner(self) -> None:
        game_object = Class(name="GameObject", namespace="")
        info = classify_arg(
            "gd::vector<gd::vector<gd::vector<GameObject*>*>*>",
            {"GameObject": game_object},
        )
        assert info is not None
        push_text = "".join(push_value(info, "sections", owner_expr="self"))
        self.assertIn("pushNestedObjectGridPointers<GameObject>", push_text)
        self.assertIn(", self)", push_text)


class GjGridBromaIntegrationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        bindings = resolve_test_bindings_dir(deps=("bindings-audit", "bindings-src"))
        if bindings is None:
            raise unittest.SkipTest(
                "Broma bindings not found, set LUAUAPI_BINDINGS_DIR or build once"
            )
        with warnings.catch_warnings():
            warnings.simplefilter("ignore")
            cls.root = collect_bindings_root(bindings, geode_sdk_path=None)
        cls.objects = {c.name: c for c in cls.root.classes}
        lookup = build_class_lookup(cls.root.classes)
        cls.gj = lookup.get("GJBaseGameLayer")
        if cls.gj is None:
            raise unittest.SkipTest("GJBaseGameLayer not in bindings")

    def test_gj_grid_fields_bindable(self) -> None:
        assert self.gj is not None
        for name in _GJ_BINDABLE_FIELDS:
            with self.subTest(field=name):
                field = next(f for f in self.gj.fields if f.name == name)
                ok, reason, _, ret = bindable_field(field, self.objects, self.gj)
                self.assertTrue(ok, reason)
                assert ret is not None
                self.assertNotIn("skipped", ret.lua_type)

    def test_gj_void_fields_still_skip(self) -> None:
        assert self.gj is not None
        for name in _GJ_STILL_SKIPPED:
            with self.subTest(field=name):
                field = next(f for f in self.gj.fields if f.name == name)
                ok, reason, _, _ = bindable_field(field, self.objects, self.gj)
                self.assertFalse(ok)
                self.assertTrue(reason.startswith("unsupported-arg:"))

    def test_baseline_field_docs_present(self) -> None:
        self.assertIn("m_nonEffectObjectsFlags", BASELINE_NESTED_BOOL_VECTOR_FIELDS)
        self.assertIn("m_collisionBlockSections", BASELINE_NESTED_OBJECT_VECTOR_FIELDS)
        self.assertIn("m_sections", BASELINE_NESTED_OBJECT_GRID_FIELDS)
        self.assertIn("m_spawnRemapTriggers", BASELINE_MAP_VECTOR_FIELDS)
        self.assertIn("m_spawnTuples", BASELINE_TUPLE_SET_FIELDS)


class GjGridFieldEmitTests(unittest.TestCase):
    def test_nested_bool_field_emits_assign(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        gj = Class(
            name="GJBaseGameLayer",
            namespace="",
            bases=["CCLayer"],
            fields=[Field("m_nonEffectObjectsFlags", "gd::vector<gd::vector<bool>*>")],
        )
        objects = {"CCObject": ccobject, "GJBaseGameLayer": gj}
        grouped, skipped = group_supported(gj, objects, "win")
        self.assertEqual(skipped, [])
        text = _emit_class_file(
            gj,
            grouped,
            [],
            [(gj, gj.fields[0])],
            objects,
            set(),
            1,
            "win",
        )
        self.assertIn("assignNestedPrimitiveVectorPointers<bool>", text)
        self.assertIn("checkNestedPrimitiveVectorPointers<bool>", text)

    def test_nested_object_field_emits_owner_getter(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        game_object = Class(name="GameObject", namespace="")
        gj = Class(
            name="GJBaseGameLayer",
            namespace="",
            bases=["CCLayer"],
            fields=[
                Field(
                    "m_collisionBlockSections",
                    "gd::vector<gd::vector<GameObject*>*>",
                )
            ],
        )
        objects = {
            "CCObject": ccobject,
            "GameObject": game_object,
            "GJBaseGameLayer": gj,
        }
        grouped, skipped = group_supported(gj, objects, "win")
        self.assertEqual(skipped, [])
        text = _emit_class_file(
            gj,
            grouped,
            [],
            [(gj, gj.fields[0])],
            objects,
            set(),
            1,
            "win",
        )
        self.assertIn("pushNestedObjectVectorPointers<GameObject>", text)
        self.assertIn("assignNestedObjectVectorPointers<GameObject>", text)
        self.assertIn(", self)", text)

    def test_map_vector_field_emits_assign(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        gj = Class(
            name="GJBaseGameLayer",
            namespace="",
            bases=["CCLayer"],
            fields=[
                Field(
                    "m_spawnRemapTriggers",
                    "gd::vector<gd::unordered_map<int,int>>",
                )
            ],
        )
        objects = {"CCObject": ccobject, "GJBaseGameLayer": gj}
        grouped, skipped = group_supported(gj, objects, "win")
        self.assertEqual(skipped, [])
        text = _emit_class_file(
            gj,
            grouped,
            [],
            [(gj, gj.fields[0])],
            objects,
            set(),
            1,
            "win",
        )
        self.assertIn("assignPrimitiveVector", text)
        self.assertIn("checkPrimitiveVector<gd::unordered_map<int, int>>", text)

    def test_tuple_set_field_emits_assign(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        gj = Class(
            name="GJBaseGameLayer",
            namespace="",
            bases=["CCLayer"],
            fields=[Field("m_spawnTuples", "gd::set<std::tuple<int, int, int>>")],
        )
        objects = {"CCObject": ccobject, "GJBaseGameLayer": gj}
        grouped, skipped = group_supported(gj, objects, "win")
        self.assertEqual(skipped, [])
        text = _emit_class_file(
            gj,
            grouped,
            [],
            [(gj, gj.fields[0])],
            objects,
            set(),
            1,
            "win",
        )
        self.assertIn("detail::assignSetContainer", text)
        self.assertIn("checkSetFromTable<std::tuple<int, int, int>", text)

    def test_nested_object_grid_field_emits_assign(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        game_object = Class(name="GameObject", namespace="")
        gj = Class(
            name="GJBaseGameLayer",
            namespace="",
            bases=["CCLayer"],
            fields=[
                Field(
                    "m_sections",
                    "gd::vector<gd::vector<gd::vector<GameObject*>*>*>",
                )
            ],
        )
        objects = {
            "CCObject": ccobject,
            "GameObject": game_object,
            "GJBaseGameLayer": gj,
        }
        grouped, skipped = group_supported(gj, objects, "win")
        self.assertEqual(skipped, [])
        text = _emit_class_file(
            gj,
            grouped,
            [],
            [(gj, gj.fields[0])],
            objects,
            set(),
            1,
            "win",
        )
        self.assertIn("pushNestedObjectGridPointers<GameObject>", text)
        self.assertIn("assignNestedObjectGridPointers<GameObject>", text)
        self.assertIn("checkNestedObjectGridPointers<GameObject>", text)
        self.assertIn(", self)", text)


if __name__ == "__main__":
    unittest.main()
