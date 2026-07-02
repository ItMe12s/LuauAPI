from __future__ import annotations

import unittest

import test_support
from luau_codegen.convert.type_map import TypeInfo, classify_return  # type: ignore[import-unresolved]
from luau_codegen.emit.bindings.class_file import _emit_invoke  # type: ignore[import-unresolved]
from luau_codegen.emit.luau_types.method_types import _method_return_type  # type: ignore[import-unresolved]
from luau_codegen.parse.broma import Arg, Class, Method  # type: ignore[import-unresolved]
from luau_codegen.policy.containers import (  # type: ignore[import-unresolved]
    container_supported_as_arg,
    container_supported_as_return,
)
from luau_codegen.policy.filtering import supported  # type: ignore[import-unresolved]
from test_support import all_platforms


def _value_primitive_vector(
    cxx: str, *, is_out: bool = False, is_vector_ptr: bool = False
) -> TypeInfo:
    return TypeInfo(
        kind="primitive_vector",
        cxx_type=cxx,
        lua_type="{ PulseEffectAction }",
        is_out=is_out,
        is_vector_ptr=is_vector_ptr,
        element_type=TypeInfo(
            kind="value",
            cxx_type="PulseEffectAction",
            lua_type="PulseEffectAction",
        ),
    )


class OutRefPolicyUnitTests(unittest.TestCase):
    def test_vector_ptr_value_return_allowed(self) -> None:
        info = _value_primitive_vector("gd::vector<PulseEffectAction>*", is_vector_ptr=True)
        self.assertTrue(container_supported_as_return(info))

    def test_vector_ptr_number_return_still_rejected(self) -> None:
        info = TypeInfo(
            kind="primitive_vector",
            cxx_type="gd::vector<int>*",
            lua_type="{ number }",
            is_vector_ptr=True,
            element_type=TypeInfo(kind="number", cxx_type="int", lua_type="number"),
        )
        self.assertFalse(container_supported_as_return(info))

    def test_nonvoid_out_value_vector_arg_allowed(self) -> None:
        info = _value_primitive_vector("gd::vector<PulseEffectAction>", is_out=True)
        self.assertTrue(container_supported_as_arg(info, "int"))

    def test_nonvoid_out_number_vector_arg_still_rejected(self) -> None:
        info = TypeInfo(
            kind="primitive_vector",
            cxx_type="gd::vector<int>",
            lua_type="{ number }",
            is_out=True,
            element_type=TypeInfo(kind="number", cxx_type="int", lua_type="number"),
        )
        self.assertFalse(container_supported_as_arg(info, "bool"))

    def test_void_out_arg_any_container_supported(self) -> None:
        info = TypeInfo(
            kind="primitive_vector",
            cxx_type="gd::vector<int>",
            lua_type="{ number }",
            is_out=True,
            element_type=TypeInfo(kind="number", cxx_type="int", lua_type="number"),
        )
        self.assertTrue(container_supported_as_arg(info, "void"))

    def test_map_out_ref_return_allowed(self) -> None:
        info = classify_return("gd::map<std::pair<int,int>, FMODSoundTween>&", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertTrue(container_supported_as_return(info))

    def test_set_ptr_return_still_rejected(self) -> None:
        info = classify_return("gd::set<unsigned int>*", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertFalse(container_supported_as_return(info))


class OutRefEmitTests(unittest.TestCase):
    def _base_layer(self) -> Class:
        return Class(name="GJBaseGameLayer", bases=["CCObject"])

    def test_register_spawn_remap_emits_multivalue_return(self) -> None:
        cls = self._base_layer()
        method = Method(
            name="registerSpawnRemap",
            ret="int",
            args=[Arg("gd::vector<ChanceObject>&", "spawnRemap")],
            platforms=all_platforms("0x1"),
        )
        text = _emit_invoke(cls, method, {}, "")
        self.assertIn("pushPrimitiveVector<ChanceObject>", text)
        self.assertIn("return 2", text)
        self.assertNotIn("return 1;", text)

    def test_get_custom_enter_effects_emits_pointer_push(self) -> None:
        cls = self._base_layer()
        method = Method(
            name="getCustomEnterEffects",
            ret="gd::vector<EnterEffectInstance>*",
            args=[
                Arg("int", "id"),
                Arg("bool", "enter"),
            ],
            platforms=all_platforms("0x1"),
        )
        text = _emit_invoke(cls, method, {}, "")
        self.assertIn("pushPrimitiveVector<EnterEffectInstance>", text)
        self.assertIn("return 1", text)

    def test_method_return_type_multivalue(self) -> None:
        cls = self._base_layer()
        method = Method(
            name="registerSpawnRemap",
            ret="int",
            args=[Arg("gd::vector<ChanceObject>&", "spawnRemap")],
            platforms=all_platforms("0x1"),
        )
        ret_type = _method_return_type(cls, method, {}, ctx=None)
        self.assertEqual(ret_type, "(number, { ChanceObject })")

    def test_method_return_type_scalar_when_no_out_arg(self) -> None:
        cls = self._base_layer()
        method = Method(
            name="simpleInt",
            ret="int",
            args=[Arg("int", "x")],
            platforms=all_platforms("0x1"),
        )
        ret_type = _method_return_type(cls, method, {}, ctx=None)
        self.assertEqual(ret_type, "number")

    def test_get_tween_container_emits_pointer_push(self) -> None:
        cls = Class(name="FMODAudioEngine", bases=["CCObject"])
        method = Method(
            name="getTweenContainer",
            ret="gd::map<std::pair<int,int>, FMODSoundTween>&",
            args=[Arg("int", "target")],
            platforms=all_platforms("0x1"),
        )
        text = _emit_invoke(cls, method, {}, "")
        self.assertIn("pushPairKeyAssociativeMapPointer", text)
        self.assertIn("&result", text)
        self.assertIn("return 1", text)
        self.assertNotIn("return 2", text)

    def test_map_out_ref_return_type(self) -> None:
        cls = Class(name="FMODAudioEngine", bases=["CCObject"])
        method = Method(
            name="getTweenContainer",
            ret="gd::map<std::pair<int,int>, FMODSoundTween>&",
            args=[Arg("int", "target")],
            platforms=all_platforms("0x1"),
        )
        ret_type = _method_return_type(cls, method, {}, ctx=None)
        self.assertEqual(
            ret_type,
            "{ { first: number, second: number, value: FMODSoundTween } }",
        )

    def test_set_ptr_return_does_not_double_address(self) -> None:
        info = classify_return("gd::set<unsigned int>*", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertFalse(container_supported_as_return(info))

    def test_set_ptr_return_emit_uses_bare_result_if_supported(self) -> None:
        cls = Class(name="CCBMFontConfiguration", bases=["CCObject"])
        method = Method(
            name="parseConfigFile",
            ret="gd::set<unsigned int>*",
            args=[Arg("gd::string", "path")],
            platforms=all_platforms("0x1"),
        )
        ok, reason = supported(cls, method, {}, "win")
        self.assertFalse(ok)
        self.assertIn("unsupported-return", reason)


if __name__ == "__main__":
    unittest.main()
