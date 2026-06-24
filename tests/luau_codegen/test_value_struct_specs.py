from __future__ import annotations

import unittest

from luau_codegen.emit.types_binding import _emit_check_struct, _emit_push_struct
from luau_codegen.emit.value_struct_specs import (
    collect_value_struct_specs,
    emit_specs_py,
)
from luau_codegen.model.value_types import CocosValueStructDescriptor
from luau_codegen.parse.broma import Class, Field, Root


def _cls(name: str, fields: list[tuple[str, str]]) -> Class:
    return Class(
        name=name,
        fields=[Field(name=n, type=t) for t, n in fields],
    )


_STUB_CLASSES = [
    Class(name="GameObject", fields=[]),
]

_FLAT_FIELDS = [
    ("int", "m_count"),
    ("float", "m_speed"),
    ("bool", "m_enabled"),
]
_NESTED_FIELDS = [
    ("cocos2d::CCPoint", "m_pos"),
    ("cocos2d::ccColor3B", "m_color"),
    ("int", "m_id"),
]
_OBJECT_FIELDS = [
    ("GameObject*", "m_target"),
    ("int", "m_groupID"),
]
_CONTAINER_FIELDS = [
    ("gd::vector<int>", "m_keys"),
    ("int", "m_value"),
]


def _root_with(*classes: Class) -> Root:
    return Root(classes=list(_STUB_CLASSES) + list(classes))


def _with_opt_in(names: tuple[str, ...]):
    from luau_codegen.model import value_struct_gate as gate

    class _Patch:
        def __enter__(self):
            self._saved = gate.VALUE_STRUCT_OPT_IN
            gate.VALUE_STRUCT_OPT_IN = names
            return self

        def __exit__(self, *exc):
            gate.VALUE_STRUCT_OPT_IN = self._saved

    return _Patch()


class CollectValueStructSpecsTests(unittest.TestCase):
    def test_flat_struct_produces_primitive_descriptors(self) -> None:
        with _with_opt_in(("FlatStruct",)):
            root = _root_with(_cls("FlatStruct", _FLAT_FIELDS))
            specs = {s.lua_name: s for s in collect_value_struct_specs(root)}
        self.assertEqual(set(specs), {"FlatStruct"})
        flat = specs["FlatStruct"]
        self.assertIsNotNone(flat.cocos)
        assert flat.cocos is not None
        kinds = [f.kind for f in flat.cocos.check_fields]
        self.assertEqual(kinds, ["integer", "number", "bool"])

    def test_derives_all_member_kinds(self) -> None:
        with _with_opt_in(("FlatStruct", "NestedStruct", "ObjectStruct", "ContainerStruct")):
            root = _root_with(
                _cls("FlatStruct", _FLAT_FIELDS),
                _cls("NestedStruct", _NESTED_FIELDS),
                _cls("ObjectStruct", _OBJECT_FIELDS),
                _cls("ContainerStruct", _CONTAINER_FIELDS),
            )
            specs = {s.lua_name: s for s in collect_value_struct_specs(root)}

        self.assertEqual(
            set(specs),
            {"FlatStruct", "NestedStruct", "ObjectStruct", "ContainerStruct"},
        )

        nested = specs["NestedStruct"]
        self.assertIsNotNone(nested.cocos)
        assert nested.cocos is not None
        nested_kinds = [f.kind for f in nested.cocos.check_fields]
        self.assertEqual(nested_kinds, ["nested", "nested", "integer"])
        self.assertEqual(nested.cocos.check_fields[0].nested_type, "cocos2d::CCPoint")
        self.assertIn("CCPoint", nested.stub_deps)

        obj = specs["ObjectStruct"]
        self.assertIsNotNone(obj.cocos)
        assert obj.cocos is not None
        obj_kinds = [f.kind for f in obj.cocos.check_fields]
        self.assertEqual(obj_kinds, ["object_nullable", "integer"])
        self.assertEqual(obj.cocos.check_fields[0].cxx_type, "GameObject")
        self.assertIn("GameObject", obj.stub_deps)

        container = specs["ContainerStruct"]
        self.assertIsNotNone(container.cocos)
        assert container.cocos is not None
        container_kinds = [f.kind for f in container.cocos.check_fields]
        self.assertEqual(container_kinds, ["container", "integer"])
        check_text = "".join(container.cocos.check_fields[0].check_override)
        self.assertIn("checkPrimitiveVector<int>", check_text)

    def test_missing_opt_in_name_records_warning(self) -> None:
        with _with_opt_in(("DoesNotExist",)):
            root = _root_with()
            specs = collect_value_struct_specs(root)
        self.assertEqual(specs, ())
        self.assertTrue(any("DoesNotExist" in w for w in root.scan_warnings))


class EmitValueStructCheckPushTests(unittest.TestCase):
    def _spec_for(self, fields: list[tuple[str, str]], name: str):
        with _with_opt_in((name,)):
            root = _root_with(_cls(name, fields))
            specs = collect_value_struct_specs(root)
        self.assertEqual(len(specs), 1)
        return specs[0]

    def _cocos_for(self, fields: list[tuple[str, str]], name: str) -> CocosValueStructDescriptor:
        spec = self._spec_for(fields, name)
        self.assertIsNotNone(spec.cocos)
        assert spec.cocos is not None
        return spec.cocos

    def test_nested_check_uses_luax_check_with_pop(self) -> None:
        desc = self._cocos_for(_NESTED_FIELDS, "NestedEmit")
        text = _emit_check_struct(desc)
        self.assertIn("NestedEmit value{};", text)
        self.assertIn('lua_getfield(L, idx, "m_pos")', text)
        self.assertIn("luax::check<cocos2d::CCPoint>(L, -1, method)", text)
        self.assertIn("lua_pop(L, 1)", text)
        self.assertIn("return value;", text)

    def test_object_nullable_check_has_nil_guard(self) -> None:
        desc = self._cocos_for(_OBJECT_FIELDS, "ObjectEmit")
        text = _emit_check_struct(desc)
        self.assertIn("if (lua_isnil(L, -1))", text)
        self.assertIn("luax::Usertype<GameObject>::check(L, -1, method)", text)
        self.assertIn("value.m_target = nullptr;", text)

    def test_object_nullable_push_pushes_borrowed_or_nil(self) -> None:
        desc = self._cocos_for(_OBJECT_FIELDS, "ObjectPush")
        param = desc.push_fields[0].member.split(".", 1)[0]
        text = _emit_push_struct(desc, param)
        self.assertIn("if (value.m_target == nullptr)", text)
        self.assertIn("luax::Usertype<GameObject>::pushBorrowed(L, value.m_target)", text)
        self.assertIn('lua_setfield(L, -2, "m_target")', text)

    def test_container_push_uses_primitive_vector_helper(self) -> None:
        desc = self._cocos_for(_CONTAINER_FIELDS, "ContainerPush")
        param = desc.push_fields[0].member.split(".", 1)[0]
        text = _emit_push_struct(desc, param)
        self.assertIn("luax::pushPrimitiveVector<int>(L, value.m_keys)", text)

    def test_integer_check_casts_to_int(self) -> None:
        desc = self._cocos_for(_FLAT_FIELDS, "FlatEmit")
        text = _emit_check_struct(desc)
        self.assertIn(
            'value.m_count = static_cast<int>(fieldNumber(L, idx, "m_count", method));',
            text,
        )

    def test_emit_specs_py_round_trips_via_repr(self) -> None:
        spec = self._spec_for(_FLAT_FIELDS, "RoundTrip")
        text = emit_specs_py((spec,))
        self.assertIn("from luau_codegen.model.value_types import", text)
        self.assertIn("VALUE_STRUCT_SPECS: tuple[ValueTypeSpec, ...] = (", text)
        self.assertIn("RoundTrip", text)
        ns: dict = {}
        exec(text, ns)
        self.assertEqual(len(ns["VALUE_STRUCT_SPECS"]), 1)
        self.assertEqual(ns["VALUE_STRUCT_SPECS"][0].lua_name, "RoundTrip")


if __name__ == "__main__":
    unittest.main()
