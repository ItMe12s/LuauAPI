from __future__ import annotations

import unittest
import warnings

from test_support import resolve_test_bindings_dir
from luau_codegen.convert.marshalling import (  # type: ignore[import-unresolved]
    emit_stack_check,
    push_value,
)
from luau_codegen.convert.type_map import classify_arg  # type: ignore[import-unresolved]
from luau_codegen.model.domain import build_class_lookup  # type: ignore[import-unresolved]
from luau_codegen.parse.broma import Class, Field  # type: ignore[import-unresolved]
from luau_codegen.parse.collect import collect_bindings_root  # type: ignore[import-unresolved]
from luau_codegen.policy.fields import bindable_field  # type: ignore[import-unresolved]
from luau_codegen.emit.bindings.class_file import _emit_class_file  # type: ignore[import-unresolved]
from luau_codegen.policy.filtering import group_supported  # type: ignore[import-unresolved]


_GJ_BINDABLE_FIELDS = (
    "m_varianceValues",
    "m_sectionSizes",
    "m_nonEffectObjectsSizes",
    "m_collisionBlockSectionSizes",
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

_AUDITED_POINTER_GRIDS = (
    (
        "m_sectionSizes",
        "gd::vector<gd::vector<int>*>",
        "gd::vector<gd::vector<int>>",
        False,
    ),
    (
        "m_nonEffectObjectsSizes",
        "gd::vector<gd::vector<int>*>",
        "gd::vector<gd::vector<int>>",
        False,
    ),
    (
        "m_collisionBlockSectionSizes",
        "gd::vector<gd::vector<int>*>",
        "gd::vector<gd::vector<int>>",
        False,
    ),
    (
        "m_nonEffectObjectsFlags",
        "gd::vector<gd::vector<bool>*>",
        "gd::vector<gd::vector<bool>>",
        True,
    ),
    (
        "m_collisionBlockSections",
        "gd::vector<gd::vector<GameObject*>*>",
        "gd::vector<gd::vector<GameObject*>>",
        True,
    ),
    (
        "m_sections",
        "gd::vector<gd::vector<gd::vector<GameObject*>*>*>",
        "gd::vector<gd::vector<gd::vector<GameObject*>>>",
        True,
    ),
    (
        "m_nonEffectObjects",
        "gd::vector<gd::vector<gd::vector<GameObject*>*>*>",
        "gd::vector<gd::vector<gd::vector<GameObject*>>>",
        True,
    ),
)


class GjGridFieldTypeMapTests(unittest.TestCase):
    def test_variance_array_classifies(self) -> None:
        info = classify_arg("std::array<float, 2000>", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "std_array")
        self.assertEqual(info.array_size, 2000)

    def test_audited_pointer_grids_store_pointer_free_mirror(self) -> None:
        game_object = Class(name="GameObject", namespace="")
        objects = {"GameObject": game_object}
        for field_name, actual, mirror, _ in _AUDITED_POINTER_GRIDS:
            with self.subTest(field=field_name):
                info = classify_arg(
                    actual,
                    objects,
                    owner_class="GJBaseGameLayer",
                    field_name=field_name,
                )
                self.assertIsNotNone(info)
                assert info is not None and info.element_type is not None
                self.assertEqual(info.kind, "audited_pointer_grid")
                self.assertEqual(info.cxx_type, actual)
                self.assertEqual(info.element_type.cxx_type, mirror)
                self.assertEqual(info.lua_type, info.element_type.lua_type)

    def test_pointer_grid_outside_exact_allowlist_is_rejected(self) -> None:
        game_object = Class(name="GameObject", namespace="")
        objects = {"GameObject": game_object}
        cases = (
            (
                "gd::vector<gd::vector<int>*>",
                "OtherLayer",
                "m_sectionSizes",
            ),
            (
                "gd::vector<gd::vector<int>*>",
                "GJBaseGameLayer",
                "m_otherSizes",
            ),
            (
                "gd::vector<gd::vector<bool>*>",
                "GJBaseGameLayer",
                "m_sectionSizes",
            ),
            (
                "gd::vector<gd::vector<float>*>",
                "GJBaseGameLayer",
                "m_sectionSizes",
            ),
            (
                "gd::vector<gd::vector<GameObject*>*>",
                "",
                "",
            ),
        )
        for actual, owner, field_name in cases:
            with self.subTest(actual=actual, owner=owner, field=field_name):
                self.assertIsNone(
                    classify_arg(
                        actual,
                        objects,
                        owner_class=owner,
                        field_name=field_name,
                    )
                )


class GjGridFieldMarshallingTests(unittest.TestCase):
    def test_audited_pointer_grid_uses_generic_runtime_adapter(self) -> None:
        game_object = Class(name="GameObject", namespace="")
        info = classify_arg(
            "gd::vector<gd::vector<gd::vector<GameObject*>*>*>",
            {"GameObject": game_object},
            owner_class="GJBaseGameLayer",
            field_name="m_sections",
        )
        assert info is not None
        push_text = "".join(push_value(info, "sections", owner_expr="self"))
        check_text = "".join(emit_stack_check(info, 2, "value", "GJBaseGameLayer.m_sections"))
        self.assertEqual(push_text.strip(), "luax::pushAuditedPointerGrid(L, sections, self);")
        self.assertIn(
            "checkAuditedPointerGrid<gd::vector<gd::vector<gd::vector<GameObject*>*>*>>",
            check_text,
        )


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


class GjGridFieldEmitTests(unittest.TestCase):
    def test_audited_pointer_grid_fields_use_one_adapter_and_exact_policy(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        game_object = Class(name="GameObject", namespace="")
        for field_name, actual, mirror, writable in _AUDITED_POINTER_GRIDS:
            with self.subTest(field=field_name):
                field = Field(field_name, actual)
                gj = Class(
                    name="GJBaseGameLayer",
                    namespace="",
                    bases=["CCLayer"],
                    fields=[field],
                )
                objects = {
                    "CCObject": ccobject,
                    "GameObject": game_object,
                    "GJBaseGameLayer": gj,
                }
                text = _emit_class_file(
                    gj,
                    {},
                    [],
                    [(gj, field)],
                    objects,
                    set(),
                    1,
                    "win",
                )
                self.assertIn(
                    f"pushAuditedPointerGrid(L, self->{field_name}, self)",
                    text,
                )
                if writable:
                    self.assertIn(f"checkAuditedPointerGrid<{actual}>", text)
                    self.assertIn(f"T* self, {mirror} value", text)
                    self.assertIn("assignAuditedPointerGrid", text)
                    self.assertIn(
                        f"field_set_{field_name}_impl(L, self, std::move(value))",
                        text,
                    )
                    self.assertIn(f"field_set_{field_name}", text)
                    self.assertNotIn("readonlyField", text)
                else:
                    self.assertIn("readonlyField", text)
                    self.assertNotIn(f"field_set_{field_name}", text)
                    self.assertNotIn("checkAuditedPointerGrid", text)
                    self.assertNotIn("assignAuditedPointerGrid", text)

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
        self.assertIn("checkContainerValue<gd::vector<gd::unordered_map<int, int>>>", text)
        self.assertIn("assignContainerValue(self->m_spawnRemapTriggers, std::move(value))", text)

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
        self.assertIn("checkContainerValue<gd::set<std::tuple<int, int, int>>>", text)
        self.assertIn("assignContainerValue(self->m_spawnTuples, std::move(value))", text)


if __name__ == "__main__":
    unittest.main()
