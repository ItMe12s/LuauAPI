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
    def test_gd_string_return_uses_check_in_apply_fn(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[
                Method(
                    name="getName",
                    ret="gd::string",
                    args=[],
                    platforms=all_platforms("0x1"),
                )
            ],
        )

        text = _emit_class_file(
            cls,
            {"getName": cls.methods},
            [(cls, cls.methods[0])],
            [],
            {"CCObject": ccobject, "CCNode": cls},
            set(),
            1,
            "win",
        )

        self.assertIn("luaapi_apply_return_CCNode_getName_0", text)
        self.assertIn("luax::check<std::string>", text)
        self.assertIn("*ctx->result = valueOverride", text)


class HookApplyFnTests(unittest.TestCase):
    def test_zero_arg_hook_uses_null_apply_args_ctx(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[
                Method(
                    name="init",
                    ret="bool",
                    args=[],
                    platforms=all_platforms("0x1"),
                )
            ],
        )

        text = _emit_class_file(
            cls,
            {"init": cls.methods},
            [(cls, cls.methods[0])],
            [],
            {"CCObject": ccobject, "CCNode": cls},
            set(),
            1,
            "win",
        )

        self.assertIn("luaapi_apply_args_CCNode_init_0", text)
        self.assertIn("hook args expected empty table", text)
        self.assertIn(
            "applyHookOverride(L, idx, &luaapi_apply_args_CCNode_init_0, nullptr",
            text,
        )
        self.assertNotIn("ApplyArgsCtx_CCNode_init_0", text)
        self.assertIn("ApplyReturnCtx_CCNode_init_0", text)

    def test_apply_args_fn_strict_table_shape(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[
                Method(
                    name="setTag",
                    ret="void",
                    args=[Arg("int", "tag")],
                    platforms=all_platforms("0x1"),
                )
            ],
        )

        text = _emit_class_file(
            cls,
            {"setTag": cls.methods},
            [(cls, cls.methods[0])],
            [],
            {"CCObject": ccobject, "CCNode": cls},
            set(),
            1,
            "win",
        )

        self.assertIn('luaL_error(L, "hook args expected table")', text)
        self.assertIn('luaL_error(L, "hook args table shape invalid")', text)

    def test_object_override_allows_nil(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[
                Method(
                    name="addChild",
                    ret="void",
                    args=[Arg("cocos2d::CCNode*", "child")],
                    platforms=all_platforms("0x1"),
                )
            ],
        )

        text = _emit_class_file(
            cls,
            {"addChild": cls.methods},
            [(cls, cls.methods[0])],
            [],
            {
                "CCObject": ccobject,
                "CCNode": cls,
                "cocos2d::CCNode": cls,
            },
            set(),
            1,
            "win",
        )

        self.assertIn("if (lua_isnil(L, -1))", text)
        self.assertIn("arg0Override = nullptr", text)


class HookableSelCallbackTests(unittest.TestCase):
    def test_sel_arg_not_hookable(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccnode = Class(name="CCNode", namespace="cocos2d", bases=["CCObject"])
        cls = Class(
            name="CCMenuItem",
            namespace="cocos2d",
            bases=["CCNode"],
            methods=[
                Method(
                    name="initWithTarget",
                    ret="bool",
                    args=[
                        Arg("cocos2d::CCObject*", "target"),
                        Arg("SEL_MenuHandler", "selector"),
                    ],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        objects = {
            "CCObject": ccobject,
            "CCNode": ccnode,
            "CCMenuItem": cls,
            "cocos2d::CCObject": ccobject,
            "cocos2d::CCNode": ccnode,
            "cocos2d::CCMenuItem": cls,
        }

        self.assertFalse(hookable(cls, cls.methods[0], objects, "win"))

    def test_callback_arg_not_hookable(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[
                Method(
                    name="runActionLater",
                    ret="void",
                    args=[Arg("std::function<void(cocos2d::CCObject*)>", "fn")],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        objects = {"CCObject": ccobject, "CCNode": cls}

        self.assertFalse(hookable(cls, cls.methods[0], objects, "win"))

    def test_vector_arg_not_hookable(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        game_object = Class(name="GameObject", bases=["CCObject"])
        cls = Class(
            name="GJBaseGameLayer",
            bases=["CCObject"],
            methods=[
                Method(
                    name="collisionCheckObjects",
                    ret="void",
                    args=[
                        Arg("GameObject*", "object"),
                        Arg("gd::vector<GameObject*>*", "objects"),
                    ],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        objects = {
            "CCObject": ccobject,
            "GameObject": game_object,
            "GJBaseGameLayer": cls,
        }

        self.assertFalse(hookable(cls, cls.methods[0], objects, "win"))
