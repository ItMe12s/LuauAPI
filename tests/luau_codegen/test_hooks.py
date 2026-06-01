from __future__ import annotations

from conftest import *


class HookOffsetTests(unittest.TestCase):
    def test_android32_uses_android32_offset(self) -> None:
        method = Method(
            name="init",
            ret="bool",
            args=[],
            platforms={"android32": "0x1234", "android64": "0x5678"},
        )

        self.assertEqual(hook_offset(method, "android32"), "0x1234")

    def test_android64_uses_android64_offset(self) -> None:
        method = Method(
            name="init",
            ret="bool",
            args=[],
            platforms={"android32": "0x1234", "android64": "0x5678"},
        )

        self.assertEqual(hook_offset(method, "android64"), "0x5678")

    def test_aggregate_android_emits_no_hook_offset(self) -> None:
        method = Method(
            name="init",
            ret="bool",
            args=[],
            platforms={"android32": "0x1234", "android64": "0x5678"},
        )

        self.assertEqual(hook_offset(method, "android"), "")

    def test_hook_offset_accepts_only_valid_hex_tokens(self) -> None:
        valid = Method(name="ok", ret="void", args=[], platforms={"win": "0xAb12"})
        self.assertEqual(hook_offset(valid, "win"), "0xAb12")

        for token in ("0xGG", "0x1);evil", "0x"):
            method = Method(name="bad", ret="void", args=[], platforms={"win": token})
            self.assertEqual(hook_offset(method, "win"), "")

    def test_android_symbol_uses_itanium_name(self) -> None:
        cls = Class(name="CCObject", namespace="cocos2d")
        method = Method(name="setTag", ret="void", args=[Arg("int", "tag")])

        self.assertEqual(
            android_symbol(cls, method),
            "_ZN7cocos2d8CCObject6setTagEi",
        )

    def test_android_symbol_preserves_pointer_const(self) -> None:
        cls = Class(name="Foo")
        method = Method(name="setText", ret="void", args=[Arg("char const*", "text")])

        self.assertEqual(android_symbol(cls, method), "_ZN3Foo7setTextEPKc")

    def test_android_linked_hook_uses_dlsym_address(self) -> None:
        cls = Class(name="CCObject", namespace="cocos2d", attributes=["link(android)"])
        method = Method(name="getTag", ret="int", args=[])

        self.assertIn(
            'dlsym(luaapi_android_libcocos(), "_ZN7cocos2d8CCObject6getTagEv")',
            hook_address_expr(cls, method, "android64"),
        )

    def test_emit_hook_support_caches_android_dlopen(self) -> None:
        text = emit_hook_support()
        self.assertIn("luaapi_android_libcocos()", text)
        self.assertIn('dlopen("libcocos2dcpp.so", RTLD_NOW)', text)
        self.assertEqual(text.count('dlopen("libcocos2dcpp.so"'), 1)

    def test_emit_internal_hpp_declares_android_libcocos(self) -> None:
        text = emit_internal_hpp()
        self.assertIn("#if defined(GEODE_IS_ANDROID)", text)
        self.assertIn("void* luaapi_android_libcocos();", text)


class F1GdStringReturnOverrideTests(unittest.TestCase):
    def test_gd_string_return_uses_var_param(self) -> None:
        info = TypeInfo(kind="string", lua_type="string", cxx_type="gd::string")
        lines = emit_return_override(info, "my_var", "-1", "test")
        text = "".join(lines)
        self.assertIn("my_var = gd::string(", text)
        self.assertNotIn("result = gd::string(", text)
