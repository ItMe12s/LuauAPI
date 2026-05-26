from __future__ import annotations

import os
import sys
import unittest


ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CODEGEN_DIR = os.path.join(ROOT, "tools", "luau_codegen")
if CODEGEN_DIR not in sys.path:
    sys.path.insert(0, CODEGEN_DIR)

from broma_parser import Arg, Class, Method  # type: ignore[import-unresolved]
from emit_luau_bindings import _emit_class_file, _emit_common_file  # type: ignore[import-unresolved]
from filtering import supported  # type: ignore[import-unresolved]
from hooks import android_symbol, emit_hook_support, hook_address_expr, hook_offset  # type: ignore[import-unresolved]


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
            'dlsym(dlopen("libcocos2dcpp.so", RTLD_NOW), "_ZN7cocos2d8CCObject6getTagEv")',
            hook_address_expr(cls, method, "android64"),
        )


class LinkClassFilterTests(unittest.TestCase):
    def test_missing_address_rejected_without_class_link(self) -> None:
        cls = Class(name="Foo")
        method = Method(name="bar", ret="void", args=[])

        ok, reason = supported(cls, method, {}, "win")

        self.assertFalse(ok)
        self.assertEqual(reason, "missing-address")

    def test_missing_address_allowed_for_link_class(self) -> None:
        cls = Class(name="CCScheduler", attributes=["link(win, android)"])
        method = Method(name="update", ret="void", args=[Arg("float", "dt")])

        ok, reason = supported(cls, method, {"CCScheduler": cls}, "win")

        self.assertTrue(ok)
        self.assertEqual(reason, "")

    def test_missing_address_still_rejected_for_unlinked_platform(self) -> None:
        cls = Class(name="CCScheduler", attributes=["link(android)"])
        method = Method(name="update", ret="void", args=[Arg("float", "dt")])

        ok, reason = supported(cls, method, {"CCScheduler": cls}, "win")

        self.assertFalse(ok)
        self.assertEqual(reason, "missing-address")

    def test_missing_platform_rejected_on_win(self) -> None:
        cls = Class(name="TestLinkClass", attributes=["link(win, android)"])
        method = Method(
            name="setupVBO",
            ret="void",
            args=[],
            attributes=["missing(win, android)"],
        )

        ok, reason = supported(cls, method, {"TestLinkClass": cls}, "win")

        self.assertFalse(ok)
        self.assertEqual(reason, "missing-platform")

    def test_missing_platform_allowed_on_mac(self) -> None:
        cls = Class(name="TestLinkClass", attributes=["link(win, mac)"])
        method = Method(
            name="setupVBO",
            ret="void",
            args=[],
            attributes=["missing(win, android)"],
        )

        ok, reason = supported(cls, method, {"TestLinkClass": cls}, "mac")

        self.assertTrue(ok)
        self.assertEqual(reason, "")

    def test_underscore_internal_rejected_on_link_class(self) -> None:
        cls = Class(name="CCImage", attributes=["link(win, android)"])
        method = Method(name="_saveImageToJPG", ret="bool", args=[Arg("char const*", "path")])

        ok, reason = supported(cls, method, {"CCImage": cls}, "win")

        self.assertFalse(ok)
        self.assertEqual(reason, "inaccessible")


class GeneratedSafetyTests(unittest.TestCase):
    def test_register_type_error_is_propagated(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        foo = Class(name="Foo", bases=["CCObject"])

        text = _emit_class_file(
            foo,
            {},
            [],
            {"CCObject": ccobject, "Foo": foo},
            set(),
            1,
            "win",
        )

        self.assertIn("auto registerResult = luax::Usertype<Foo>::registerType", text)
        self.assertIn("if (registerResult.isErr()) return geode::Err(registerResult.unwrapErr());", text)

    def test_common_file_asserts_userdata_tag_budget(self) -> None:
        text = _emit_common_file([Class(name="CCObject", namespace="cocos2d")])

        self.assertIn("static_assert(1 < LUA_UTAG_LIMIT", text)

    def test_hook_shutdown_clears_refs_and_disables_hooks(self) -> None:
        text = emit_hook_support()

        self.assertIn("callback->removed = true;", text)
        self.assertIn("callback->ref.reset();", text)
        self.assertIn("state.hook->disable();", text)
        self.assertIn("!callback || callback->removed", text)


if __name__ == "__main__":
    unittest.main()
