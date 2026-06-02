from __future__ import annotations

from dataclasses import dataclass

import unittest
from helpers import (
    Arg,  # type: ignore[import-unresolved]
    Class,  # type: ignore[import-unresolved]
    Method,  # type: ignore[import-unresolved]
    _emit_class_file,  # type: ignore[import-unresolved]
    all_platforms,  # type: ignore[import-unresolved]
    android_symbol,  # type: ignore[import-unresolved]
    emit_hook_support,  # type: ignore[import-unresolved]
    emit_internal_hpp,  # type: ignore[import-unresolved]
    hook_address_expr,  # type: ignore[import-unresolved]
    hook_offset,  # type: ignore[import-unresolved]
    hookable,  # type: ignore[import-unresolved]
)


@dataclass(frozen=True)
class _HookCb:
    priority: int
    install_order: int
    kind: int = 0


def _pre_hook_less(a: _HookCb, b: _HookCb) -> bool:
    if a.priority != b.priority:
        return a.priority < b.priority
    return a.install_order < b.install_order


def _post_hook_less(a: _HookCb, b: _HookCb) -> bool:
    if a.priority != b.priority:
        return a.priority > b.priority
    return a.install_order > b.install_order


def _insert_sorted(items: list[_HookCb], item: _HookCb, less) -> None:
    lo = 0
    hi = len(items)
    while lo < hi:
        mid = (lo + hi) // 2
        if less(items[mid], item):
            lo = mid + 1
        else:
            hi = mid
    items.insert(lo, item)


def _build_sorted(installs: list[_HookCb], *, pre: bool) -> list[_HookCb]:
    less = _pre_hook_less if pre else _post_hook_less
    sorted_list: list[_HookCb] = []
    for item in installs:
        _insert_sorted(sorted_list, item, less)
    return sorted_list


class HookRuntimeOrderTests(unittest.TestCase):
    def test_pre_hook_fire_order_matches_runtime_pre_sorted_list(self) -> None:
        installs = [
            _HookCb(priority=10, install_order=0, kind=0),
            _HookCb(priority=5, install_order=1, kind=0),
            _HookCb(priority=5, install_order=2, kind=0),
            _HookCb(priority=0, install_order=3, kind=0),
        ]
        fire_order = [cb.install_order for cb in _build_sorted(installs, pre=True)]
        self.assertEqual(fire_order, [3, 1, 2, 0])

    def test_post_hook_fire_order_matches_runtime_post_sorted_list(self) -> None:
        installs = [
            _HookCb(priority=10, install_order=0, kind=1),
            _HookCb(priority=5, install_order=1, kind=1),
            _HookCb(priority=5, install_order=2, kind=1),
            _HookCb(priority=0, install_order=3, kind=1),
        ]
        fire_order = [cb.install_order for cb in _build_sorted(installs, pre=False)]
        self.assertEqual(fire_order, [0, 2, 1, 3])

    def test_emit_hook_support_uses_matching_priority_comparators(self) -> None:
        text = emit_hook_support()
        self.assertIn("return a->priority < b->priority", text)
        self.assertIn("return a->priority > b->priority", text)
        self.assertIn("return a->installOrder < b->installOrder", text)
        self.assertIn("return a->installOrder > b->installOrder", text)
        self.assertIn(
            "auto const& callbacks = it->second.preSorted", emit_internal_hpp()
        )
        self.assertIn(
            "auto const& callbacks = it->second.postSorted", emit_internal_hpp()
        )


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

    def test_emit_internal_hpp_presorts_hook_callbacks_at_install(self) -> None:
        header = emit_internal_hpp()
        support = emit_hook_support()
        self.assertIn("preSorted", header)
        self.assertIn("postSorted", header)
        self.assertIn("insertPreSorted", support)
        self.assertNotIn("std::stable_sort(callbacks.begin()", header)

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

    def test_table_copy_containers_not_hookable(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="Foo",
            bases=["CCObject"],
            methods=[
                Method(
                    name="takeInts",
                    ret="void",
                    args=[Arg("gd::vector<int>", "values")],
                    platforms=all_platforms("0x1"),
                ),
                Method(
                    name="getMap",
                    ret="gd::map<int, bool>",
                    args=[],
                    platforms=all_platforms("0x2"),
                ),
            ],
        )
        objects = {"CCObject": ccobject, "Foo": cls}

        for method in cls.methods:
            self.assertFalse(hookable(cls, method, objects, "win"))

    def test_opaque_handle_arg_not_hookable(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="FMODAudioEngine",
            bases=["CCObject"],
            attributes=["link(win)"],
            methods=[
                Method(
                    name="stopChannel",
                    ret="void",
                    args=[Arg("FMOD::Channel*", "channel")],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        objects = {"CCObject": ccobject, "FMODAudioEngine": cls}

        self.assertFalse(hookable(cls, cls.methods[0], objects, "win"))
