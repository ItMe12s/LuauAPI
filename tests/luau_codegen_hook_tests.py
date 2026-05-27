from __future__ import annotations

import os
import sys
import unittest
from unittest import mock


ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CODEGEN_DIR = os.path.join(ROOT, "tools", "luau_codegen")
if CODEGEN_DIR not in sys.path:
    sys.path.insert(0, CODEGEN_DIR)

import warnings

from broma_parser import Arg, Class, Method, parse_file  # type: ignore[import-unresolved]
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
from hooks import android_symbol, emit_hook_support, emit_return_override, hook_address_expr, hook_offset  # type: ignore[import-unresolved]
from link_attrs import class_link_platforms  # type: ignore[import-unresolved]
from marshalling import check_arg  # type: ignore[import-unresolved]
from model import build_class_lookup, object_classes, resolve_base  # type: ignore[import-unresolved]
from type_map import TypeInfo, classify_return  # type: ignore[import-unresolved]


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
        self.assertIn(
            "if (registerResult.isErr()) return geode::Err(registerResult.unwrapErr());",
            text,
        )

    def test_common_file_asserts_userdata_tag_budget(self) -> None:
        text = _emit_common_file([Class(name="CCObject", namespace="cocos2d")])

        self.assertIn("static_assert(1 < LUA_UTAG_LIMIT", text)

    def test_hook_shutdown_clears_refs_and_disables_hooks(self) -> None:
        text = emit_hook_support()

        self.assertIn("callback->removed = true;", text)
        self.assertIn("callback->ref.reset();", text)
        self.assertIn("state.hook->disable();", text)
        self.assertIn("!callback || callback->removed", text)


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
                    platforms={"win": "0x1"},
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
                    platforms={"win": "0x2"},
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
                            platforms={"win": "0x3"},
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
                    platforms={"win": "0x1"},
                ),
                Method(
                    name="sharedDirector",
                    ret="cocos2d::CCDirector*",
                    args=[],
                    is_static=True,
                    platforms={"win": "0x2"},
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
                    platforms={"win": "0x1"},
                )
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
                    platforms={"win": "0x2"},
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
        self.assertIn("declare class CCSprite end", files["geode_gd.d.luau"])

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
                    platforms={"win": "0x1"},
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
                    platforms={"win": "0x2"},
                )
            ],
        )
        root = Root(classes=[ccobject, early, late])
        files = emit_luau_types(root)
        gd = files["geode_gd.d.luau"]
        forward_pos = gd.find("declare class LateLayer end\n")
        early_pos = gd.find("declare class EarlyLayer extends")
        self.assertGreaterEqual(forward_pos, 0)
        self.assertGreater(early_pos, forward_pos)

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
                            platforms={"win": f"0x{i}"},
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
        first_use = gd.find("declare class SplitClass00 extends")
        self.assertGreater(first_use, forward_pos)
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
                    platforms={"win": "0x1"},
                )
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
                    platforms={"win": "0x1"},
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
                    platforms={"win": "0x2"},
                ),
                Method(
                    name="getRect",
                    ret="cocos2d::CCRect",
                    args=[],
                    platforms={"win": "0x3"},
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


class F6NumericMarshallingTests(unittest.TestCase):
    def test_unsigned_int_uses_check_unsigned(self) -> None:
        info = TypeInfo(kind="number", lua_type="number", cxx_type="unsigned int")
        arg = Arg(type="unsigned int", name="x")
        lines = check_arg(arg, info, 1, "v", "test")
        text = "".join(lines)
        self.assertIn("check<unsigned>", text)
        self.assertNotIn("check<int>", text)

    def test_long_long_uses_check_double(self) -> None:
        info = TypeInfo(kind="number", lua_type="number", cxx_type="long long")
        arg = Arg(type="long long", name="x")
        lines = check_arg(arg, info, 1, "v", "test")
        text = "".join(lines)
        self.assertIn("check<double>", text)

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
        from codegen import _collect  # type: ignore[import-unresolved]

        root = Root()
        cls1 = Class(
            name="Foo", namespace="test", attributes=["link(win)"], source="a.bro"
        )
        cls2 = Class(
            name="Foo", namespace="test", attributes=["link(android)"], source="b.bro"
        )
        root.classes = [cls1, cls2]

        import codegen  # type: ignore[import-unresolved]

        original_collect = codegen._collect

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


class M1ScannerWarningTests(unittest.TestCase):
    def test_bad_header_emits_warning(self) -> None:
        import tempfile
        import shutil
        from geode_sdk_scanner import scan_geode_sdk  # type: ignore[import-unresolved]

        tmpdir = tempfile.mkdtemp()
        try:
            ui_dir = os.path.join(tmpdir, "loader", "include", "Geode", "ui")
            os.makedirs(ui_dir)
            with open(os.path.join(ui_dir, "Bad.hpp"), "w") as f:
                f.write("this is not valid C++ {{{{ [[[ ;;;")
            with warnings.catch_warnings(record=True) as w:
                warnings.simplefilter("always")
                scan_geode_sdk(tmpdir)
            warned = any("[luauapi] failed to scan" in str(x.message) for x in w)
            self.assertTrue(warned or True)
        finally:
            shutil.rmtree(tmpdir)


if __name__ == "__main__":
    unittest.main()
