from __future__ import annotations

import os
import shutil
import sys
import tempfile
import unittest
from unittest import mock


ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CODEGEN_DIR = os.path.join(ROOT, "tools", "luau_codegen")
if CODEGEN_DIR not in sys.path:
    sys.path.insert(0, CODEGEN_DIR)

import warnings

from broma_parser import Arg, Class, Field, Method, parse_file  # type: ignore[import-unresolved]
from emit_luau_bindings import (  # type: ignore[import-unresolved]
    _emit_class_file,
    _emit_common_file,
    _emit_dispatcher,
    _input_arg_count,
)
from emit_luau_types import emit as emit_luau_types  # type: ignore[import-unresolved]
from broma_parser import Root  # type: ignore[import-unresolved]
from type_map import classify_arg, is_const_reference, is_out_reference  # type: ignore[import-unresolved]
from filtering import group_supported, is_link_platform, supported  # type: ignore[import-unresolved]
from hooks import (  # type: ignore[import-unresolved]
    android_symbol,
    emit_hook_support,
    emit_return_override,
    emit_value_decode,
    hook_address_expr,
    hook_offset,
    hookable,
)
from emit_luau_bindings import emit as emit_luau_bindings  # type: ignore[import-unresolved]
from intersection import intersection_platforms  # type: ignore[import-unresolved]
from codegen import collect_bindings_root  # type: ignore[import-unresolved]
from codegen import main as codegen_main  # type: ignore[import-unresolved]
from cxx_templates import emit_internal_hpp  # type: ignore[import-unresolved]
from link_attrs import class_link_platforms  # type: ignore[import-unresolved]
from marshalling import check_arg, push_value  # type: ignore[import-unresolved]
from model import build_class_lookup, codegen_object_map, object_classes, resolve_base  # type: ignore[import-unresolved]
from parity import collect_parity, emit_markdown  # type: ignore[import-unresolved]
from plan import collect_plan, collect_platform_plan, plan_outputs  # type: ignore[import-unresolved]
from type_map import TypeInfo  # type: ignore[import-unresolved]


def all_platforms(value: str = "0x1") -> dict[str, str]:
    return {
        "win": value,
        "m1": value,
        "ios": value,
        "android32": value,
        "android64": value,
    }


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
        method = Method(
            name="_saveImageToJPG", ret="bool", args=[Arg("char const*", "path")]
        )

        ok, reason = supported(cls, method, {"CCImage": cls}, "win")

        self.assertFalse(ok)
        self.assertEqual(reason, "inaccessible")

    def test_game_level_manager_has_liked_item_full_check_is_denied(self) -> None:
        cls = Class(name="GameLevelManager", attributes=["link(android)"])
        method = Method(
            name="hasLikedItemFullCheck",
            ret="bool",
            args=[
                Arg("LikeItemType", "type"),
                Arg("int", "id"),
                Arg("bool", "liked"),
                Arg("int", "parentID"),
            ],
            platforms={
                "win": "0x164d80",
                "imac": "0x557a90",
                "m1": "0x4a78e0",
                "ios": "0xaa3f4",
            },
        )

        ok, reason = supported(
            cls,
            method,
            {"GameLevelManager": cls, "LikeItemType": Class(name="LikeItemType")},
            "android64",
        )

        self.assertFalse(ok)
        self.assertEqual(reason, "inaccessible")


class AccessLevelTests(unittest.TestCase):
    def test_protected_method_rejected(self) -> None:
        cls = Class(name="CCTextFieldTTF", attributes=["link(win)"])
        method = Method(
            name="insertText",
            ret="void",
            args=[
                Arg("char const*", "text"),
                Arg("int", "len"),
                Arg("cocos2d::enumKeyCodes", "key"),
            ],
            access="protected",
            platforms={"win": "link"},
        )

        ok, reason = supported(cls, method, {"CCTextFieldTTF": cls}, "win")

        self.assertFalse(ok)
        self.assertEqual(reason, "inaccessible")

    def test_parse_protected_prefix(self) -> None:
        import tempfile

        bro = """
class Foo {
    protected virtual void insertText(char const* text, int len);
};
"""
        with tempfile.NamedTemporaryFile("w", suffix=".bro", delete=False) as f:
            f.write(bro)
            path = f.name
        try:
            root = parse_file(path)
            methods = root.classes[0].methods
            self.assertEqual(len(methods), 1)
            self.assertEqual(methods[0].name, "insertText")
            self.assertEqual(methods[0].access, "protected")
        finally:
            os.unlink(path)

    def test_parse_access_section(self) -> None:
        import tempfile

        bro = """
class Foo {
protected:
    virtual void insertText(char const* text);
};
"""
        with tempfile.NamedTemporaryFile("w", suffix=".bro", delete=False) as f:
            f.write(bro)
            path = f.name
        try:
            root = parse_file(path)
            methods = root.classes[0].methods
            self.assertEqual(len(methods), 1)
            self.assertEqual(methods[0].access, "protected")
        finally:
            os.unlink(path)


class CompoundLinkAttributeTests(unittest.TestCase):
    def test_compound_link_attribute_matches_android64(self) -> None:
        cls = Class(
            name="GameManager",
            attributes=["link(android)", "depends(UIButtonConfig)"],
        )

        self.assertTrue(is_link_platform(cls, "android64"))
        self.assertFalse(is_link_platform(cls, "win"))

    def test_compound_link_attribute_string_matches_android64(self) -> None:
        cls = Class(
            name="GameManager",
            attributes=["link(android), depends(UIButtonConfig)"],
        )

        self.assertEqual(class_link_platforms(cls), {"android"})
        self.assertTrue(is_link_platform(cls, "android64"))

    def test_linked_game_manager_method_supported_on_android64(self) -> None:
        cls = Class(
            name="GameManager",
            attributes=["link(android)", "depends(UIButtonConfig)"],
        )
        method = Method(
            name="update",
            ret="void",
            args=[Arg("float", "dt")],
            platforms={"win": "0x189bc0", "imac": "0x38bb20"},
        )

        ok, reason = supported(cls, method, {"GameManager": cls}, "android64")

        self.assertTrue(ok)
        self.assertEqual(reason, "")


class BareInlineTests(unittest.TestCase):
    def test_bare_inline_callable_on_win(self) -> None:
        cls = Class(name="GameManager")
        method = Method(
            name="getPlayLayer",
            ret="PlayLayer*",
            args=[],
            platforms={"inline": "inline"},
        )

        ok, reason = supported(
            cls,
            method,
            {"GameManager": cls, "PlayLayer": Class(name="PlayLayer")},
            "win",
        )

        self.assertTrue(ok)
        self.assertEqual(reason, "")

    def test_platform_inline_callable_on_ios(self) -> None:
        cls = Class(name="SimplePlayer", bases=["cocos2d::CCSprite"])
        method = Method(
            name="setColors",
            ret="void",
            args=[
                Arg("cocos2d::ccColor3B const&", "color1"),
                Arg("cocos2d::ccColor3B const&", "color2"),
            ],
            platforms={
                "win": "inline",
                "imac": "0x36edb0",
                "m1": "0x2f8c50",
                "ios": "inline",
            },
        )
        objects = {
            "SimplePlayer": cls,
            "cocos2d::CCSprite": Class(name="CCSprite", namespace="cocos2d"),
        }

        ok, reason = supported(cls, method, objects, "ios")

        self.assertTrue(ok)
        self.assertEqual(reason, "")

    def test_platform_inline_survives_intersection(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="SimplePlayer",
            bases=["cocos2d::CCSprite"],
            methods=[
                Method(
                    name="setColors",
                    ret="void",
                    args=[
                        Arg("cocos2d::ccColor3B const&", "color1"),
                        Arg("cocos2d::ccColor3B const&", "color2"),
                    ],
                    platforms={
                        "win": "inline",
                        "imac": "0x36edb0",
                        "m1": "0x2f8c50",
                        "ios": "inline",
                        "android32": "0x1",
                        "android64": "0x1",
                    },
                )
            ],
        )
        root = Root(
            classes=[
                ccobject,
                Class(name="CCSprite", namespace="cocos2d", bases=["CCObject"]),
                cls,
            ]
        )

        plan = collect_plan(root, "win")

        self.assertIn("setColors", plan.supported_by_class["SimplePlayer"])


class GeneratedSafetyTests(unittest.TestCase):
    def test_register_type_error_is_propagated(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        foo = Class(name="Foo", bases=["CCObject"])

        text = _emit_class_file(
            foo,
            {},
            [],
            [],
            {"CCObject": ccobject, "Foo": foo},
            set(),
            1,
            "win",
        )

        self.assertIn("auto registerResult = luax::Usertype<Foo>::registerType", text)
        self.assertIn(
            "if (registerResult.isErr()) return geode::Err(registerResult.unwrapErr());",
            text,
        )

    def test_common_file_asserts_userdata_tag_budget(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccnode = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[
                Method(
                    name="getTag",
                    ret="int",
                    args=[],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        root = Root(classes=[ccobject, ccnode])
        plan = collect_plan(root, "win")
        text = _emit_common_file(plan.emitted_classes, plan, "win")

        self.assertIn("static_assert(1 < LUA_UTAG_LIMIT", text)

    def test_hook_shutdown_clears_refs_and_disables_hooks(self) -> None:
        text = emit_hook_support()

        self.assertIn("callback->removed = true;", text)
        self.assertIn("callback->ref.reset();", text)
        self.assertIn("state.hook->disable();", text)
        self.assertIn("!callback || callback->removed", text)

    def test_hook_runtime_uses_named_deadline(self) -> None:
        text = emit_internal_hpp()

        self.assertIn("luax::kHookScriptDeadlineMs", text)
        self.assertNotIn("targetId, 50", text)

    def test_hook_api_is_table_only_with_modify_skip_fields(self) -> None:
        text = emit_hook_support()

        self.assertIn("geode.hook expected callback table", text)
        self.assertNotIn("geode.hook expected function at arg 2", text)
        self.assertIn("luaapi_geode_modify", text)
        self.assertIn("luaapi_geode_skip", text)
        self.assertIn("luaapi_geode_fields", text)

    def test_invalid_skip_value_continues_before_chain(self) -> None:
        text = emit_internal_hpp()

        self.assertIn('returned invalid skip value", targetId', text)
        self.assertIn("continue;", text)
        self.assertIn("return true;", text)

    def test_ui_button_config_decode_supported_for_hook_overrides(self) -> None:
        info = TypeInfo(
            kind="value", cxx_type="UIButtonConfig", lua_type="UIButtonConfig"
        )
        text = "".join(emit_value_decode(info, "config", "-1", "hook args"))

        self.assertIn("luax::toUIButtonConfig", text)
        self.assertIn("config = ", text)

    def test_generated_hook_thunk_runs_pre_branch(self) -> None:
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

        self.assertIn("runLuaPreHooks", text)
        self.assertIn("skipOriginal", text)
        self.assertIn("lua_objlen", text)
        self.assertIn("useArrayArgs", text)
        self.assertIn('lua_getfield(L, idx, "tag")', text)
        self.assertIn("arg0 = arg0Override", text)
        self.assertIn("if (!skipOriginal)", text)

    def test_hook_named_args_disabled_for_duplicate_names(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[
                Method(
                    name="setPair",
                    ret="void",
                    args=[Arg("int", "value"), Arg("int", "value")],
                    platforms=all_platforms("0x1"),
                )
            ],
        )

        text = _emit_class_file(
            cls,
            {"setPair": cls.methods},
            [(cls, cls.methods[0])],
            [],
            {"CCObject": ccobject, "CCNode": cls},
            set(),
            1,
            "win",
        )

        self.assertIn("bool useNamedArgs = false;", text)

    def test_luau_owned_attr_pushes_owned_instance_return(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[
                Method(
                    name="newChild",
                    ret="cocos2d::CCNode*",
                    args=[],
                    attributes=["luau_owned"],
                    platforms=all_platforms("0x1"),
                )
            ],
        )

        text = _emit_class_file(
            cls,
            {"newChild": cls.methods},
            [],
            [],
            {"CCObject": ccobject, "CCNode": cls, "cocos2d::CCNode": cls},
            set(),
            1,
            "win",
        )

        self.assertIn("pushOwned", text)

    def test_unmarked_instance_return_stays_borrowed(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[
                Method(
                    name="child",
                    ret="cocos2d::CCNode*",
                    args=[],
                    platforms=all_platforms("0x1"),
                )
            ],
        )

        text = _emit_class_file(
            cls,
            {"child": cls.methods},
            [],
            [],
            {"CCObject": ccobject, "CCNode": cls, "cocos2d::CCNode": cls},
            set(),
            1,
            "win",
        )

        self.assertIn("pushBorrowed", text)
        self.assertNotIn("pushOwned", text)

    def test_generated_field_accessors_are_registered(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            fields=[Field("m_nTag", "int")],
        )

        text = _emit_class_file(
            cls,
            {},
            [],
            [(cls, cls.fields[0])],
            {"CCObject": ccobject, "CCNode": cls},
            set(),
            1,
            "win",
        )

        self.assertIn("luaapi_CCNode_field_get_m_nTag", text)
        self.assertIn("self->m_nTag", text)
        self.assertIn("luaapi_CCNode_field_register_m_nTag<cocos2d::CCNode>(L);", text)
        self.assertIn('luax::Usertype<T>::field(L, "m_nTag"', text)

    def test_field_plan_and_types_include_ccnode_m_fields_only(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccnode = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            fields=[Field("m_obPosition", "cocos2d::CCPoint")],
        )
        root = Root(classes=[ccobject, ccnode])

        plan = collect_plan(root, "win")
        files = emit_luau_types(root, "win", plan=plan)

        self.assertEqual(len(plan.field_targets_by_class["CCNode"]), 1)
        self.assertIn("m_fields: { [string]: any }", files["geode_cocos2d.d.luau"])
        self.assertIn("m_obPosition: CCPoint", files["geode_cocos2d.d.luau"])
        self.assertIn("declare class CCObject end", files["geode_cocos2d.d.luau"])
        self.assertNotIn("declare class CCPoint end", files["geode_cocos2d.d.luau"])

    def test_inaccessible_broma_field_is_not_bound(self) -> None:
        ccobject = Class(
            name="CCObject",
            namespace="cocos2d",
            fields=[Field("m_nChildIndex", "int")],
        )
        root = Root(classes=[ccobject])

        plan = collect_plan(root, "win")
        files = emit_luau_types(root, "win", plan=plan)

        self.assertEqual(plan.field_targets_by_class.get("CCObject", []), [])
        self.assertIn(
            "-- skipped m_nChildIndex: inaccessible-field",
            files["geode_cocos2d.d.luau"],
        )

    def test_inaccessible_field_type_is_not_bound(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccnode = Class(name="CCNode", namespace="cocos2d", bases=["CCObject"])
        batch_node = Class(
            name="CCParticleBatchNode",
            namespace="cocos2d",
            bases=["CCNode"],
        )
        particle_system = Class(
            name="CCParticleSystem",
            namespace="cocos2d",
            bases=["CCNode"],
            fields=[Field("m_pBatchNode", "CCParticleBatchNode*")],
        )
        root = Root(classes=[ccobject, ccnode, batch_node, particle_system])

        plan = collect_plan(root, "win")
        files = emit_luau_types(root, "win", plan=plan)

        self.assertEqual(plan.field_targets_by_class.get("CCParticleSystem", []), [])
        self.assertIn(
            "-- skipped m_pBatchNode: inaccessible-type:CCParticleBatchNode",
            files["geode_cocos2d.d.luau"],
        )


class LuauTypeEmissionTests(unittest.TestCase):
    def test_enum_prelude_uses_valid_luau_names(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccnode = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[
                Method(
                    name="getPosition",
                    ret="cocos2d::CCPoint",
                    args=[],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        gm = Class(
            name="GameManager",
            bases=["GManager"],
            methods=[
                Method(
                    name="colorForIdx",
                    ret="cocos2d::ccColor3B",
                    args=[Arg("int", "idx")],
                    platforms=all_platforms("0x2"),
                )
            ],
        )
        root = Root(
            classes=[
                ccobject,
                ccnode,
                Class(
                    name="GManager",
                    bases=["CCNode"],
                    methods=[
                        Method(
                            name="init",
                            ret="bool",
                            args=[],
                            platforms=all_platforms("0x3"),
                        )
                    ],
                ),
                gm,
            ]
        )
        files = emit_luau_types(root)
        cocos = files["geode_cocos2d.d.luau"]
        gd = files["geode_gd.d.luau"]

        self.assertNotIn("cocos2d::", cocos)
        self.assertIn("export type enumKeyCodes = number", cocos)
        self.assertNotIn("export type IconType = number", cocos)
        self.assertIn("export type RGBColor = { r: number, g: number, b: number }", gd)
        self.assertIn("declare class CCNode end", gd)

    def test_factories_live_in_namespace_files(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        director = Class(
            name="CCDirector",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[
                Method(
                    name="getWinSize",
                    ret="cocos2d::CCSize",
                    args=[],
                    platforms=all_platforms("0x1"),
                ),
                Method(
                    name="sharedDirector",
                    ret="cocos2d::CCDirector*",
                    args=[],
                    is_static=True,
                    platforms=all_platforms("0x2"),
                ),
            ],
            attributes=["link(win)"],
        )
        root = Root(classes=[ccobject, director])
        files = emit_luau_types(root)
        cocos = files["geode_cocos2d.d.luau"]
        main = files["geode.d.luau"]

        self.assertIn(
            "export type CCDirectorFactory", files["geode_cocos2d_factories.d.luau"]
        )
        self.assertIn(
            "export type Cocos2dNamespace", files["geode_cocos2d_factories.d.luau"]
        )
        self.assertIn(
            "sharedDirector: (() -> CCDirector?)",
            files["geode_cocos2d_factories.d.luau"],
        )
        self.assertIn("function getWinSize(self): CCSize", cocos)
        self.assertNotIn("export type CCDirectorFactory", cocos)
        self.assertNotIn("export type CCDirectorFactory", main)
        self.assertIn("cocos2d: Cocos2dNamespace", main)

    def test_cross_namespace_forward_decls(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        text_area = Class(
            name="TextArea",
            bases=["CCSprite"],
            methods=[
                Method(
                    name="create",
                    ret="TextArea*",
                    args=[Arg("char const*", "text")],
                    is_static=True,
                    platforms=all_platforms("0x1"),
                ),
                Method(
                    name="init",
                    ret="bool",
                    args=[],
                    platforms=all_platforms("0x2"),
                ),
            ],
        )
        ccnode = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[
                Method(
                    name="addTextArea",
                    ret="void",
                    args=[Arg("TextArea*", "area")],
                    platforms=all_platforms("0x3"),
                )
            ],
        )
        root = Root(
            classes=[
                ccobject,
                Class(name="CCSprite", namespace="cocos2d", bases=["CCNode"]),
                ccnode,
                text_area,
            ]
        )
        files = emit_luau_types(root)
        self.assertIn("declare class TextArea end", files["geode_cocos2d.d.luau"])
        self.assertNotIn("declare class CCSprite end", files["geode_gd.d.luau"])

    def test_intra_file_forward_decls(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        early = Class(
            name="EarlyLayer",
            bases=["CCObject"],
            methods=[
                Method(
                    name="getLate",
                    ret="LateLayer*",
                    args=[],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        late = Class(
            name="LateLayer",
            bases=["CCObject"],
            methods=[
                Method(
                    name="init",
                    ret="bool",
                    args=[],
                    platforms=all_platforms("0x2"),
                )
            ],
        )
        root = Root(classes=[ccobject, early, late])
        files = emit_luau_types(root)
        gd = files["geode_gd.d.luau"]
        forward_pos = gd.find("declare class LateLayer end\n")
        early_pos = gd.find("declare class EarlyLayer")
        self.assertGreaterEqual(forward_pos, 0)
        self.assertGreaterEqual(early_pos, 0)

    def test_split_file_forward_decls(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        classes = [ccobject]
        for i in range(30):
            name = f"SplitClass{i:02d}"
            ret = "SplitClass29*" if i == 0 else "bool"
            classes.append(
                Class(
                    name=name,
                    bases=["CCObject"],
                    methods=[
                        Method(
                            name="init",
                            ret=ret,
                            args=[],
                            platforms=all_platforms(f"0x{i}"),
                        )
                    ],
                )
            )
        root = Root(classes=classes)
        with mock.patch("emit_luau_types.MAX_CLASS_FILE_LINES", 80):
            files = emit_luau_types(root)
        self.assertIn("geode_gd_2.d.luau", files)
        gd = files["geode_gd.d.luau"]
        self.assertIn("declare class SplitClass29 end\n", gd)
        forward_pos = gd.find("declare class SplitClass29 end\n")
        first_use = gd.find("declare class SplitClass00")
        self.assertGreaterEqual(first_use, 0)
        self.assertIn(
            "declare class SplitClass29",
            files["geode_gd_2.d.luau"],
        )

    def test_factory_forward_decls(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        widget = Class(
            name="WidgetLayer",
            bases=["CCObject"],
            methods=[
                Method(
                    name="create",
                    ret="WidgetLayer*",
                    args=[],
                    is_static=True,
                    platforms=all_platforms("0x1"),
                ),
                Method(
                    name="init",
                    ret="bool",
                    args=[],
                    platforms=all_platforms("0x2"),
                ),
            ],
            attributes=["link(win)"],
        )
        root = Root(classes=[ccobject, widget])
        files = emit_luau_types(root)
        factories = files["geode_gd_factories.d.luau"]
        forward_pos = factories.find("declare class WidgetLayer end")
        factory_pos = factories.find("export type WidgetLayerFactory")
        self.assertGreaterEqual(forward_pos, 0)
        self.assertGreater(factory_pos, forward_pos)

    def test_skipped_class_forward_decls(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccgrid_base = Class(name="CCGridBase", namespace="cocos2d", bases=["CCObject"])
        ccnode = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            fields=[Field("m_pGrid", "cocos2d::CCGridBase*")],
        )
        root = Root(classes=[ccobject, ccgrid_base, ccnode])
        files = emit_luau_types(root)
        cocos = files["geode_cocos2d.d.luau"]
        forward_pos = cocos.find("declare class CCGridBase end\n")
        node_pos = cocos.find("declare class CCNode")
        self.assertGreaterEqual(forward_pos, 0)
        self.assertGreater(node_pos, forward_pos)
        self.assertIn("m_pGrid: CCGridBase?", cocos)

    def test_orphan_forward_decls_exclude_value_types(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccnode = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            fields=[Field("m_obPosition", "cocos2d::CCPoint")],
        )
        root = Root(classes=[ccobject, ccnode])
        files = emit_luau_types(root)
        cocos = files["geode_cocos2d.d.luau"]
        self.assertIn("m_obPosition: CCPoint", cocos)
        self.assertNotIn("declare class CCPoint end", cocos)

    def test_value_stub_order(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        node = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[
                Method(
                    name="getRect",
                    ret="cocos2d::CCRect",
                    args=[],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        gm = Class(
            name="GameManager",
            bases=["CCNode"],
            methods=[
                Method(
                    name="getSize",
                    ret="cocos2d::CCSize",
                    args=[],
                    platforms=all_platforms("0x2"),
                ),
                Method(
                    name="getRect",
                    ret="cocos2d::CCRect",
                    args=[],
                    platforms=all_platforms("0x3"),
                ),
            ],
        )
        root = Root(classes=[ccobject, node, gm])
        files = emit_luau_types(root)
        gd = files["geode_gd.d.luau"]
        ccsize_pos = gd.find("export type CCSize =")
        ccrect_pos = gd.find("export type CCRect =")
        self.assertGreaterEqual(ccsize_pos, 0)
        self.assertGreater(ccrect_pos, ccsize_pos)

    def test_geode_root_namespace_stubs(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        root = Root(classes=[ccobject])
        main = emit_luau_types(root)["geode.d.luau"]
        self.assertIn("type Cocos2dNamespace = any", main)
        self.assertIn("type GDNamespace = any", main)
        self.assertIn("cocos2d: Cocos2dNamespace", main)
        self.assertIn("before: ((...any) -> ())?,", main)
        self.assertIn("after: ((...any) -> ())?,", main)

    def test_dispatcher_uses_input_arg_count_for_out_refs(self) -> None:
        method = Method(
            name="getUnlockForAchievement",
            ret="void",
            args=[
                Arg("char const*", "achievement"),
                Arg("UnlockType&", "type"),
            ],
        )
        objects = {"UnlockType": Class(name="UnlockType")}

        self.assertEqual(_input_arg_count(method, objects), 1)

    def test_const_ref_is_input_not_out(self) -> None:
        self.assertTrue(is_const_reference("cocos2d::ccColor3B const&"))
        self.assertFalse(is_out_reference("cocos2d::ccColor3B const&"))
        self.assertTrue(is_out_reference("UnlockType&"))

        info = classify_arg("cocos2d::ccColor3B const&", {})
        assert info is not None
        self.assertTrue(info.is_ref)
        self.assertFalse(info.is_out)

    def test_const_ref_overload_dispatcher_has_distinct_cases(self) -> None:
        cls = Class(name="GameObject", bases=["CCSprite"])
        methods = [
            Method(
                name="updateMainColor",
                ret="void",
                args=[Arg("cocos2d::ccColor3B const&", "color")],
                platforms={"win": "0x1"},
            ),
            Method(
                name="updateMainColor",
                ret="void",
                args=[],
                platforms={"win": "inline"},
            ),
        ]
        objects = {
            "GameObject": cls,
            "CCSprite": Class(name="CCSprite", namespace="cocos2d"),
        }
        text = _emit_dispatcher(cls, "updateMainColor", methods, objects)
        self.assertIn("case 0:", text)
        self.assertIn("case 1:", text)
        self.assertNotIn(
            "case 0: return luaapi_GameObject_updateMainColor_0(L);\n            case 0:",
            text,
        )


class F1GdStringReturnOverrideTests(unittest.TestCase):
    def test_gd_string_return_uses_var_param(self) -> None:
        info = TypeInfo(kind="string", lua_type="string", cxx_type="gd::string")
        lines = emit_return_override(info, "my_var", "-1", "test")
        text = "".join(lines)
        self.assertIn("my_var = gd::string(", text)
        self.assertNotIn("result = gd::string(", text)


class F3MethodAttrSplitTests(unittest.TestCase):
    def test_compound_method_attrs_split(self) -> None:
        from broma_parser import _Cursor, _parse_class_body  # type: ignore[import-unresolved]

        cls = Class(name="Foo", namespace="test")
        bro = "[[link(win), missing(android)]] void bar() = win 0x1234; }"
        cursor = _Cursor(bro)
        _parse_class_body(cursor, cls)
        self.assertTrue(len(cls.methods) > 0)
        m = cls.methods[0]
        self.assertIn("link(win)", m.attributes)
        self.assertIn("missing(android)", m.attributes)
        self.assertEqual(len(m.attributes), 2)


class F4ClassLookupCollisionTests(unittest.TestCase):
    def test_ambiguous_short_name_resolved_by_qualified(self) -> None:
        cls_a = Class(name="Sprite", namespace="ns1", bases=["cocos2d::CCObject"])
        cls_b = Class(name="Sprite", namespace="ns2", bases=["cocos2d::CCObject"])
        lookup = build_class_lookup([cls_a, cls_b])
        self.assertIs(lookup["ns1::Sprite"], cls_a)
        self.assertIs(lookup["ns2::Sprite"], cls_b)
        self.assertNotIn("Sprite", lookup)

    def test_unambiguous_short_name_works(self) -> None:
        cls = Class(name="CCObject", namespace="cocos2d")
        lookup = build_class_lookup([cls])
        self.assertIs(lookup["CCObject"], cls)
        self.assertIs(lookup["cocos2d::CCObject"], cls)

    def test_resolve_base_tries_qualified_then_short(self) -> None:
        cls = Class(name="CCNode", namespace="cocos2d")
        lookup = build_class_lookup([cls])
        self.assertIs(resolve_base(lookup, "cocos2d::CCNode"), cls)
        self.assertIs(resolve_base(lookup, "CCNode"), cls)
        self.assertIsNone(resolve_base(lookup, "Unknown"))

    def test_codegen_object_map_drops_ambiguous_short_name(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls_a = Class(name="Sprite", namespace="ns1", bases=["cocos2d::CCObject"])
        cls_b = Class(name="Sprite", namespace="ns2", bases=["cocos2d::CCObject"])
        objects = codegen_object_map(Root(classes=[ccobject, cls_a, cls_b]))
        self.assertIs(objects["ns1::Sprite"], cls_a)
        self.assertIs(objects["ns2::Sprite"], cls_b)
        self.assertNotIn("Sprite", objects)


class F5OverloadFirstDeclaredWinsTests(unittest.TestCase):
    def test_first_overload_kept_on_arity_collision(self) -> None:
        cls = Class(
            name="TestObj",
            namespace="cocos2d",
            bases=["CCObject"],
            attributes=["link(win)"],
        )
        m1 = Method(
            name="foo",
            ret="void",
            args=[Arg(type="int", name="a")],
            platforms={"win": "0x1"},
        )
        m2 = Method(
            name="foo",
            ret="void",
            args=[Arg(type="float", name="b")],
            platforms={"win": "0x2"},
        )
        cls.methods = [m1, m2]
        objects = {"TestObj": cls, "cocos2d::TestObj": cls}
        grouped, skipped = group_supported(cls, objects, "win")
        self.assertIn("foo", grouped)
        self.assertEqual(len(grouped["foo"]), 1)
        self.assertIs(grouped["foo"][0], m1)
        self.assertEqual(len(skipped), 1)
        self.assertIn("ambiguous-overload-arity", skipped[0][1])

    def test_fail_on_ambiguous_overload_flag_exits_nonzero(self) -> None:
        tmpdir = tempfile.mkdtemp()
        try:
            with open(
                os.path.join(tmpdir, "GeometryDash.bro"), "w", encoding="utf-8"
            ) as f:
                f.write(
                    "class cocos2d::CCObject {};"
                    "class gd::TestObj : cocos2d::CCObject {"
                    "void foo(int a) = win 0x1;"
                    "void foo(float b) = win 0x2;"
                    "};"
                )
            code = codegen_main(
                [
                    "--bindings",
                    tmpdir,
                    "--platform",
                    "win",
                    "--list-outputs",
                    "--fail-on-ambiguous-overload",
                ]
            )
            self.assertEqual(code, 6)
        finally:
            shutil.rmtree(tmpdir)

    def test_ambiguous_overload_default_does_not_fail(self) -> None:
        tmpdir = tempfile.mkdtemp()
        try:
            with open(
                os.path.join(tmpdir, "GeometryDash.bro"), "w", encoding="utf-8"
            ) as f:
                f.write(
                    "class cocos2d::CCObject {};"
                    "class gd::TestObj : cocos2d::CCObject {"
                    "void foo(int a) = win 0x1;"
                    "void foo(float b) = win 0x2;"
                    "};"
                )
            code = codegen_main(
                ["--bindings", tmpdir, "--platform", "win", "--list-outputs"]
            )
            self.assertEqual(code, 0)
        finally:
            shutil.rmtree(tmpdir)


class F6NumericMarshallingTests(unittest.TestCase):
    def test_unsigned_int_uses_check_unsigned(self) -> None:
        info = TypeInfo(kind="number", lua_type="number", cxx_type="unsigned int")
        arg = Arg(type="unsigned int", name="x")
        lines = check_arg(arg, info, 1, "v", "test")
        text = "".join(lines)
        self.assertIn("check<unsigned>", text)
        self.assertNotIn("check<int>", text)

    def test_double_uses_check_double(self) -> None:
        info = TypeInfo(kind="number", lua_type="number", cxx_type="double")
        arg = Arg(type="double", name="x")
        lines = check_arg(arg, info, 1, "v", "test")
        text = "".join(lines)
        self.assertIn("check<double>", text)

    def test_wide_integer_uses_string_marshalling(self) -> None:
        info = classify_arg("uint64_t", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "wideint")
        self.assertEqual(info.lua_type, "string")
        check_text = "".join(check_arg(Arg("uint64_t", "x"), info, 1, "v", "test"))
        push_text = "".join(push_value(info, "v"))
        self.assertIn("checkIntegerString<uint64_t>", check_text)
        self.assertIn("pushIntegerString", push_text)

    def test_unsigned_push_does_not_narrow_through_int(self) -> None:
        with open(
            os.path.join(ROOT, "src", "lua", "bindings", "framework", "Stack.hpp"),
            "r",
            encoding="utf-8",
        ) as f:
            text = f.read()
        self.assertIn("inline void push(lua_State* L, unsigned v)", text)
        self.assertNotIn("static_cast<int>(v)", text)
        self.assertIn("static_cast<double>(v)", text)

    def test_regular_int_unchanged(self) -> None:
        info = TypeInfo(kind="number", lua_type="number", cxx_type="int")
        arg = Arg(type="int", name="x")
        lines = check_arg(arg, info, 1, "v", "test")
        text = "".join(lines)
        self.assertIn("check<int>", text)


class F8ConstMethodManglingTests(unittest.TestCase):
    def test_const_method_has_K_qualifier(self) -> None:
        cls = Class(name="CCObject", namespace="cocos2d")
        m = Method(
            name="getTag",
            ret="int",
            args=[],
            is_const=True,
            platforms={"android64": "0x1"},
        )
        sym = android_symbol(cls, m)
        self.assertTrue(sym.startswith("_ZNK"), f"Expected _ZNK prefix, got {sym}")
        self.assertIn("6getTag", sym)

    def test_non_const_method_no_K_qualifier(self) -> None:
        cls = Class(name="CCObject", namespace="cocos2d")
        m = Method(
            name="setTag",
            ret="void",
            args=[Arg(type="int", name="t")],
            platforms={"android64": "0x1"},
        )
        sym = android_symbol(cls, m)
        self.assertTrue(sym.startswith("_ZN"), f"Expected _ZN prefix, got {sym}")
        self.assertFalse(sym.startswith("_ZNK"))

    def test_const_vs_non_const_distinct(self) -> None:
        cls = Class(name="CCObject", namespace="cocos2d")
        m_const = Method(
            name="getTag",
            ret="int",
            args=[],
            is_const=True,
            platforms={"android64": "0x1"},
        )
        m_non = Method(
            name="getTag",
            ret="int",
            args=[],
            is_const=False,
            platforms={"android64": "0x1"},
        )
        self.assertNotEqual(android_symbol(cls, m_const), android_symbol(cls, m_non))

    def test_reference_and_const_type_mangling(self) -> None:
        cls = Class(name="Foo")
        method = Method(name="take", ret="void", args=[Arg("int const&", "value")])
        self.assertEqual(android_symbol(cls, method), "_ZN3Foo4takeERKi")

    def test_repeated_pointer_uses_substitution(self) -> None:
        cls = Class(name="Bar")
        method = Method(
            name="take",
            ret="void",
            args=[Arg("Foo*", "a"), Arg("Foo*", "b")],
        )
        self.assertEqual(android_symbol(cls, method), "_ZN3Bar4takeEP3FooS0_")

    def test_template_type_mangling_includes_defaults(self) -> None:
        cls = Class(name="Foo")
        method = Method(
            name="take",
            ret="void",
            args=[Arg("gd::vector<int>", "values")],
        )
        self.assertEqual(
            android_symbol(cls, method), "_ZN3Foo4takeEN2gd6vectorIiSaIiEEE"
        )


class F9FileSplitTests(unittest.TestCase):
    def test_multi_split_produces_unique_filenames(self) -> None:
        from emit_luau_types import (  # type: ignore[import-unresolved]
            MAX_CLASS_FILE_LINES,
            _pack_line_chunks,
        )

        header = ["-- header\n"]
        big_chunk = "x\n" * (MAX_CLASS_FILE_LINES + 1)
        chunks = [(f"Cls{i}", big_chunk) for i in range(3)]
        packed, classes_per_file = _pack_line_chunks(header, chunks, "test.d.luau")
        self.assertEqual(len(packed), 3)
        self.assertIn("test.d.luau", packed)
        self.assertIn("test_2.d.luau", packed)
        self.assertIn("test_3.d.luau", packed)


class F11ClassMergeTests(unittest.TestCase):
    def test_duplicate_class_merges_attrs(self) -> None:
        root = Root()
        cls1 = Class(
            name="Foo", namespace="test", attributes=["link(win)"], source="a.bro"
        )
        cls2 = Class(
            name="Foo", namespace="test", attributes=["link(android)"], source="b.bro"
        )
        root.classes = [cls1, cls2]

        seen: dict = {}
        for cls in root.classes:
            if cls.qualified_name in seen:
                existing = seen[cls.qualified_name]
                for attr in cls.attributes:
                    if attr not in existing.attributes:
                        existing.attributes.append(attr)
            else:
                seen[cls.qualified_name] = cls
        merged = list(seen.values())
        self.assertEqual(len(merged), 1)
        self.assertIn("link(win)", merged[0].attributes)
        self.assertIn("link(android)", merged[0].attributes)

    def test_duplicate_class_warns_on_method_conflict(self) -> None:
        cls1 = Class(name="Bar", namespace="test", source="a.bro")
        cls1.methods = [
            Method(name="foo", ret="void", args=[], platforms={"win": "0x1"})
        ]
        cls2 = Class(name="Bar", namespace="test", source="b.bro")
        cls2.methods = [
            Method(name="bar", ret="void", args=[], platforms={"win": "0x2"})
        ]

        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter("always")
            seen: dict = {}
            for cls in [cls1, cls2]:
                if cls.qualified_name in seen:
                    existing = seen[cls.qualified_name]
                    for attr in cls.attributes:
                        if attr not in existing.attributes:
                            existing.attributes.append(attr)
                    if cls.methods and existing.methods:
                        warnings.warn(
                            f"[luauapi] duplicate class {cls.qualified_name} "
                            f"from {cls.source} and {existing.source}, keeping first"
                        )
                    elif cls.methods and not existing.methods:
                        existing.methods = cls.methods
                else:
                    seen[cls.qualified_name] = cls
            self.assertEqual(len(w), 1)
            self.assertIn("duplicate class", str(w[0].message))


class CodegenIoTests(unittest.TestCase):
    def test_write_if_changed_skips_identical_content(self) -> None:
        from codegen import _write_if_changed  # type: ignore[import-unresolved]

        tmpdir = tempfile.mkdtemp()
        try:
            path = os.path.join(tmpdir, "out.txt")
            _write_if_changed(path, "same\n")
            first_mtime = os.stat(path).st_mtime_ns
            _write_if_changed(path, "same\n")
            self.assertEqual(os.stat(path).st_mtime_ns, first_mtime)
            _write_if_changed(path, "different\n")
            self.assertEqual(open(path, encoding="utf-8").read(), "different\n")
        finally:
            shutil.rmtree(tmpdir)

    def test_collect_bindings_root_warns_for_missing_bro_files(self) -> None:
        from codegen import collect_bindings_root  # type: ignore[import-unresolved]

        tmpdir = tempfile.mkdtemp()
        try:
            with open(
                os.path.join(tmpdir, "GeometryDash.bro"), "w", encoding="utf-8"
            ) as f:
                f.write("class Foo {\n    void bar();\n};\n")
            with warnings.catch_warnings(record=True) as w:
                warnings.simplefilter("always")
                root = collect_bindings_root(tmpdir)
            self.assertEqual(len(root.classes), 1)
            self.assertTrue(any("missing Broma file" in str(x.message) for x in w))
        finally:
            shutil.rmtree(tmpdir)


class F12ParityReportTests(unittest.TestCase):
    def test_intersection_keeps_only_all_platform_methods(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        foo = Class(
            name="Foo",
            bases=["CCObject"],
            methods=[
                Method(name="shared", ret="void", args=[], platforms=all_platforms()),
                Method(
                    name="missingIOS",
                    ret="void",
                    args=[],
                    platforms={
                        "win": "0x1",
                        "m1": "0x1",
                        "android32": "0x1",
                        "android64": "0x1",
                    },
                ),
            ],
        )
        root = Root(classes=[ccobject, foo])

        plan = collect_plan(root, "win")

        self.assertIn("shared", plan.supported_by_class["Foo"])
        self.assertNotIn("missingIOS", plan.supported_by_class["Foo"])
        self.assertTrue(
            any(
                cls == "Foo"
                and method == "missingIOS"
                and reason == "intersection-missing-platform:ios"
                for cls, method, reason in plan.skipped
            )
        )

    def test_intersection_removes_hooks_missing_one_platform(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        foo = Class(
            name="Foo",
            bases=["CCObject"],
            methods=[
                Method(
                    name="hookMe",
                    ret="void",
                    args=[],
                    platforms={
                        "win": "0x1",
                        "m1": "0x1",
                        "ios": "0x1",
                        "android32": "inline",
                        "android64": "inline",
                    },
                )
            ],
        )
        root = Root(classes=[ccobject, foo])

        plan = collect_plan(root, "win")

        self.assertIn("hookMe", plan.supported_by_class["Foo"])
        self.assertEqual(plan.hook_targets_by_class["Foo"], [])
        self.assertEqual(len(plan.intersection_stats.removed_hooks), 1)

    def test_type_factories_use_intersection_plan(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        factory = Class(
            name="FactoryOnly",
            bases=["CCObject"],
            methods=[
                Method(
                    name="create",
                    ret="FactoryOnly*",
                    args=[],
                    is_static=True,
                    platforms={
                        "win": "0x1",
                        "m1": "0x1",
                        "android32": "0x1",
                        "android64": "0x1",
                    },
                )
            ],
        )
        root = Root(classes=[ccobject, factory])

        files = emit_luau_types(root)

        self.assertNotIn("FactoryOnlyFactory", files["geode_gd_factories.d.luau"])

    def test_class_without_intersected_members_has_no_binding_file(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        weak = Class(
            name="WeakClass",
            bases=["CCObject"],
            methods=[
                Method(
                    name="winOnly",
                    ret="void",
                    args=[],
                    platforms={"win": "0x1"},
                )
            ],
        )
        root = Root(classes=[ccobject, weak])

        outputs = plan_outputs(root, "win")
        files = emit_luau_types(root)

        self.assertNotIn("bindings_WeakClass.cpp", outputs)
        self.assertNotIn("WeakClass", files["geode_gd.d.luau"])

    def test_android_link_class_reports_missing_win_callable_proof(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        android_only = Class(
            name="AndroidOnly",
            bases=["CCObject"],
            attributes=["link(android)"],
            methods=[Method(name="ping", ret="void", args=[])],
        )
        root = Root(classes=[ccobject, android_only])

        data = collect_parity(root)
        key = "AndroidOnly.ping()"

        self.assertIn("android64", data["methods"][key]["supportedPlatforms"])
        self.assertEqual(data["methods"][key]["skipReasons"]["win"], "missing-address")
        self.assertIn(key, data["hints"]["winMissingCallableProof"])

    def test_m1_offset_method_reports_supported(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        m1_only = Class(
            name="M1Only",
            bases=["CCObject"],
            methods=[
                Method(
                    name="tick",
                    ret="void",
                    args=[],
                    platforms={"m1": "0x1234"},
                )
            ],
        )
        root = Root(classes=[ccobject, m1_only])

        data = collect_parity(root)
        key = "M1Only.tick()"

        self.assertIn("m1", data["methods"][key]["supportedPlatforms"])

    def test_collect_parity_reuses_supplied_platform_plans(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        foo = Class(
            name="Foo",
            namespace="gd",
            bases=["CCObject"],
            methods=[
                Method(name="shared", ret="void", args=[], platforms=all_platforms())
            ],
        )
        root = Root(classes=[ccobject, foo])
        platforms = ("win", "m1", "ios", "android32", "android64")
        plans = {
            platform: collect_platform_plan(root, platform) for platform in platforms
        }

        with mock.patch("parity.collect_platform_plan") as mocked_collect:
            data = collect_parity(root, platforms=platforms, plans_by_platform=plans)

        mocked_collect.assert_not_called()
        self.assertIn("gd::Foo.shared()", data["methods"])

    def test_ios_strict_callable_class_prune_is_reported(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        static_only = Class(
            name="StaticOnly",
            bases=["CCObject"],
            methods=[
                Method(
                    name="make",
                    ret="StaticOnly*",
                    args=[],
                    is_static=True,
                    platforms={"inline": "inline"},
                )
            ],
        )
        root = Root(classes=[ccobject, static_only])

        data = collect_parity(root)
        key = "StaticOnly.make()"

        self.assertIn("StaticOnly", data["skippedClasses"]["ios"])
        self.assertEqual(
            data["methods"][key]["skipReasons"]["ios"],
            "not-callable-type:ios:StaticOnly",
        )
        self.assertIn("StaticOnly", emit_markdown(data))

    def test_inaccessible_class_method_ref_is_pruned(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccnode = Class(name="CCNode", namespace="cocos2d", bases=["CCObject"])
        batch_node = Class(
            name="CCParticleBatchNode",
            namespace="cocos2d",
            bases=["CCNode"],
        )
        foo = Class(
            name="Foo",
            namespace="cocos2d",
            bases=["CCNode"],
            methods=[
                Method(
                    name="getBatch",
                    ret="CCParticleBatchNode*",
                    args=[],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        root = Root(classes=[ccobject, ccnode, batch_node, foo])

        data = collect_parity(root)
        key = "cocos2d::Foo.getBatch()"

        self.assertEqual(
            data["methods"][key]["skipReasons"]["win"],
            "not-callable-type:win:CCParticleBatchNode",
        )
        self.assertEqual(
            data["methods"][key]["skipReasons"]["ios"],
            "not-callable-type:ios:CCParticleBatchNode",
        )

    def test_hook_parity_separates_callable_from_hookable(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        inline_only = Class(
            name="InlineOnly",
            bases=["CCObject"],
            methods=[
                Method(
                    name="localCall",
                    ret="void",
                    args=[],
                    platforms={"inline": "inline"},
                )
            ],
        )
        root = Root(classes=[ccobject, inline_only])

        data = collect_parity(root)
        key = "InlineOnly.localCall()"

        self.assertIn("win", data["methods"][key]["supportedPlatforms"])
        self.assertNotIn("win", data["methods"][key]["hookablePlatforms"])
        self.assertIn(key, data["hints"]["hookAddressGaps"])


class M1ScannerWarningTests(unittest.TestCase):
    def test_bad_header_emits_warning(self) -> None:
        from geode_sdk_scanner import scan_geode_sdk  # type: ignore[import-unresolved]

        tmpdir = tempfile.mkdtemp()
        try:
            ui_dir = os.path.join(tmpdir, "loader", "include", "Geode", "ui")
            os.makedirs(ui_dir)
            include_dir = os.path.dirname(ui_dir)
            with open(os.path.join(include_dir, "UI.hpp"), "w", encoding="utf-8") as f:
                f.write('#include "ui/Bad.hpp"\n')
            with open(os.path.join(ui_dir, "Bad.hpp"), "w") as f:
                f.write("this is not valid C++ {{{{ [[[ ;;;")
            with warnings.catch_warnings(record=True) as w:
                warnings.simplefilter("always")
                with mock.patch(
                    "geode_sdk_scanner._scan_header", side_effect=ValueError("bad")
                ):
                    scan_geode_sdk(tmpdir)
            warned = any("[luauapi] failed to scan" in str(x.message) for x in w)
            self.assertTrue(warned)
        finally:
            shutil.rmtree(tmpdir)

    def test_scanned_ui_class_links_win_only(self) -> None:
        from geode_sdk_scanner import scan_geode_sdk  # type: ignore[import-unresolved]

        tmpdir = tempfile.mkdtemp()
        try:
            ui_dir = os.path.join(tmpdir, "loader", "include", "Geode", "ui")
            os.makedirs(ui_dir)
            include_dir = os.path.dirname(ui_dir)
            with open(os.path.join(include_dir, "UI.hpp"), "w", encoding="utf-8") as f:
                f.write('#include "ui/Good.hpp"\n')
            with open(os.path.join(ui_dir, "Good.hpp"), "w", encoding="utf-8") as f:
                f.write(
                    "namespace geode { "
                    "class GEODE_DLL GoodUI : public cocos2d::CCObject { "
                    "public: void show(); "
                    "}; "
                    "}"
                )
            classes = scan_geode_sdk(tmpdir)
            self.assertEqual(len(classes), 1)
            self.assertEqual(class_link_platforms(classes[0]), {"win"})

            ccobject = Class(name="CCObject", namespace="cocos2d")
            objects = {"CCObject": ccobject, "GoodUI": classes[0]}
            ok_win, _ = supported(classes[0], classes[0].methods[0], objects, "win")
            ok_android, reason = supported(
                classes[0], classes[0].methods[0], objects, "android64"
            )
            self.assertTrue(ok_win)
            self.assertFalse(ok_android)
            self.assertEqual(reason, "missing-address")
        finally:
            shutil.rmtree(tmpdir)


class PlanRegressionTests(unittest.TestCase):
    def test_intersection_platforms_uses_imac_for_intel_mac(self) -> None:
        self.assertEqual(
            intersection_platforms("imac"),
            ("win", "imac", "ios", "android32", "android64"),
        )
        self.assertEqual(
            intersection_platforms("m1"),
            ("win", "m1", "ios", "android32", "android64"),
        )

    def test_collect_parity_uses_target_mac_axis(self) -> None:
        root = Root(classes=[Class(name="CCObject", namespace="cocos2d")])
        imac_data = collect_parity(root, target_platform="imac")
        m1_data = collect_parity(root, target_platform="m1")
        self.assertIn("imac", imac_data["platforms"])
        self.assertNotIn("m1", imac_data["platforms"])
        self.assertIn("m1", m1_data["platforms"])
        self.assertNotIn("imac", m1_data["platforms"])
        self.assertEqual(imac_data["hints"]["macPlatform"], "imac")
        self.assertEqual(m1_data["hints"]["macPlatform"], "m1")
        self.assertIn("macGapReasons", imac_data["hints"])
        self.assertNotIn("m1GapReasons", imac_data["hints"])

    def test_imac_intersection_keeps_methods_without_m1_offset(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="IntelOnly",
            namespace="gd",
            bases=["cocos2d::CCObject"],
            attributes=["link(win)"],
        )
        cls.methods = [
            Method(
                name="intelMethod",
                ret="void",
                args=[],
                platforms={
                    "win": "0x1",
                    "imac": "0x2",
                    "ios": "0x3",
                    "android32": "0x4",
                    "android64": "0x5",
                },
            )
        ]
        root = Root(classes=[ccobject, cls])
        plan = collect_plan(root, "imac")
        grouped = plan.supported_by_class.get("IntelOnly", {})
        self.assertIn("intelMethod", grouped)

    def test_plan_outputs_matches_binding_emit(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="SampleNode",
            namespace="cocos2d",
            bases=["CCObject"],
            attributes=["link(win)"],
        )
        cls.methods = [
            Method(name="doThing", ret="void", args=[], platforms={"win": "0x10"})
        ]
        root = Root(classes=[ccobject, cls])
        plan = collect_plan(root, "win")
        listed = set(plan_outputs(root, "win", plan=plan))
        emitted = set(emit_luau_bindings(root, "win", plan=plan)[0].keys())
        self.assertEqual(listed, emitted)

    def test_hookable_allows_const_ref_args(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="GameObject",
            namespace="gd",
            bases=["cocos2d::CCObject"],
            attributes=["link(win)"],
        )
        method = Method(
            name="updateMainColor",
            ret="void",
            args=[Arg("cocos2d::ccColor3B const&", "color")],
            platforms={"win": "0x100"},
        )
        cls.methods = [method]
        objects = {"CCObject": ccobject, "GameObject": cls, "gd::GameObject": cls}
        self.assertTrue(hookable(cls, method, objects, "win"))

    def test_hookable_rejects_out_ref_args(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="GameManager",
            namespace="gd",
            bases=["cocos2d::CCObject"],
            attributes=["link(win)"],
        )
        method = Method(
            name="getUnlockForAchievement",
            ret="void",
            args=[
                Arg("char const*", "achievement"),
                Arg("UnlockType&", "type"),
            ],
            platforms={"win": "0x200"},
        )
        cls.methods = [method]
        objects = {
            "CCObject": ccobject,
            "GameManager": cls,
            "UnlockType": Class(name="UnlockType"),
        }
        self.assertFalse(hookable(cls, method, objects, "win"))

    def test_duplicate_class_merges_methods(self) -> None:
        tmpdir = tempfile.mkdtemp()
        try:
            bindings = os.path.join(tmpdir, "bindings")
            os.makedirs(bindings)
            bro_a = os.path.join(bindings, "Cocos2d.bro")
            bro_b = os.path.join(bindings, "Extras.bro")
            with open(bro_a, "w", encoding="utf-8") as f:
                f.write(
                    "class cocos2d::MergeTest : cocos2d::CCObject {\n"
                    "    void first() = win 0x1;\n"
                    "}\n"
                )
            with open(bro_b, "w", encoding="utf-8") as f:
                f.write(
                    "class cocos2d::MergeTest : cocos2d::CCObject {\n"
                    "    void second() = win 0x2;\n"
                    "}\n"
                )
            for name in ("FMOD.bro", "Kazmath.bro", "GeometryDash.bro"):
                open(os.path.join(bindings, name), "w", encoding="utf-8").close()
            root = collect_bindings_root(bindings)
            merged = next(c for c in root.classes if c.name == "MergeTest")
            names = {m.name for m in merged.methods}
            self.assertEqual(names, {"first", "second"})
        finally:
            shutil.rmtree(tmpdir)


class CodegenExitCodeTests(unittest.TestCase):
    def test_emit_exception_returns_exit_code_5(self) -> None:
        import codegen as cg  # type: ignore[import-unresolved]

        root = Root(classes=[Class(name="CCObject", namespace="cocos2d")])
        tmpdir = tempfile.mkdtemp()
        try:
            bindings = os.path.join(tmpdir, "bindings")
            out_dir = os.path.join(tmpdir, "out")
            types_dir = os.path.join(tmpdir, "types")
            os.makedirs(bindings)
            os.makedirs(types_dir)
            for name in (
                "Cocos2d.bro",
                "Extras.bro",
                "FMOD.bro",
                "Kazmath.bro",
                "GeometryDash.bro",
            ):
                open(os.path.join(bindings, name), "w", encoding="utf-8").close()
            with mock.patch.object(cg, "collect_bindings_root", return_value=root):
                with mock.patch.object(
                    cg.emit_luau_bindings,
                    "emit",
                    side_effect=ValueError("emit failed"),
                ):
                    rc = cg.main(
                        [
                            "--bindings",
                            bindings,
                            "--out",
                            out_dir,
                            "--types-out",
                            types_dir,
                            "--platform",
                            "win",
                        ]
                    )
            self.assertEqual(rc, 5)
        finally:
            shutil.rmtree(tmpdir)


if __name__ == "__main__":
    unittest.main()
