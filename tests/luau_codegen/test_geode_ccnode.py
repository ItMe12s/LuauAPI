from __future__ import annotations

import os
import shutil
import tempfile
import unittest

from test_support import all_platforms, types_text
from luau_codegen.convert.type_map import classify_arg, register_geode_enums  # type: ignore[import-unresolved]
from luau_codegen.emit.bindings.class_file import _emit_class_file  # type: ignore[import-unresolved]
from luau_codegen.emit.luau_types import emit as emit_luau_types  # type: ignore[import-unresolved]
from luau_codegen.emit.plan import collect_plan  # type: ignore[import-unresolved]
from luau_codegen.parse.broma import Arg, Class, Method, Root  # type: ignore[import-unresolved]
from luau_codegen.parse.collect import collect_bindings_root  # type: ignore[import-unresolved]
from luau_codegen.policy.filtering import group_supported  # type: ignore[import-unresolved]
from luau_codegen.policy.link_attrs import class_link_platforms  # type: ignore[import-unresolved]


def _write(path: str, text: str) -> None:
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        f.write(text)


def _write_empty_broma_dir(path: str) -> None:
    from luau_codegen.model.domain import BRO_FILES  # type: ignore[import-unresolved]

    os.makedirs(path, exist_ok=True)
    for name in BRO_FILES:
        _write(os.path.join(path, name), "")


def _write_ccnode_header(sdk_path: str, body: str) -> str:
    path = os.path.join(
        sdk_path,
        "loader",
        "include",
        "Geode",
        "cocos",
        "base_nodes",
        "CCNode.h",
    )
    _write(
        path,
        f"NS_CC_BEGIN\nclass CC_DLL CCNode : public CCObject\n{{\n{body}\n}};\nNS_CC_END\n",
    )
    return path


class GeodeCCNodeScannerTests(unittest.TestCase):
    def test_scans_public_geode_dll_methods_only(self) -> None:
        from luau_codegen.parse.geode_sdk import (  # type: ignore[import-unresolved]
            scan_geode_ccnode_additions,
        )

        tmpdir = tempfile.mkdtemp()
        try:
            _write_ccnode_header(
                tmpdir,
                """
public:
    void vanilla();
    GEODE_DLL geode::ZStringView getID();
    GEODE_DLL void setID(std::string id);
    GEODE_DLL void setUserFlag(std::string id, bool state = true);
    GEODE_DLL void addChildAtPosition(
        CCNode* child,
        geode::Anchor anchor,
        CCPoint const& offset = CCPointZero,
        bool useAnchorLayout = true
    );
    GEODE_DLL void withBody() {}
    template <class T>
    GEODE_DLL T* getChildByType();
    class Inner {
    public:
        GEODE_DLL void nested();
    };
protected:
    GEODE_DLL void protectedThing();
private:
    GEODE_DLL void privateThing();
""",
            )

            cls = scan_geode_ccnode_additions(tmpdir)
            self.assertIsNotNone(cls)
            assert cls is not None
            self.assertEqual(cls.qualified_name, "cocos2d::CCNode")
            self.assertEqual(cls.attributes, [])
            self.assertEqual(
                set(cls.methods[0].platforms),
                {"win", "android", "android32", "android64", "imac", "m1", "ios"},
            )

            names = [m.name for m in cls.methods]
            self.assertEqual(
                names,
                ["getID", "setID", "setUserFlag", "addChildAtPosition"],
            )
            set_flag = next(m for m in cls.methods if m.name == "setUserFlag")
            self.assertEqual([a.type for a in set_flag.args], ["std::string", "bool"])
            anchored = next(m for m in cls.methods if m.name == "addChildAtPosition")
            self.assertEqual(
                [a.type for a in anchored.args],
                ["CCNode*", "geode::Anchor", "CCPoint const&", "bool"],
            )
        finally:
            shutil.rmtree(tmpdir)

    def test_missing_ccnode_header_is_nonfatal(self) -> None:
        from luau_codegen.parse.geode_sdk import (  # type: ignore[import-unresolved]
            scan_geode_ccnode_additions,
        )

        tmpdir = tempfile.mkdtemp()
        try:
            self.assertIsNone(scan_geode_ccnode_additions(tmpdir))
        finally:
            shutil.rmtree(tmpdir)

    def test_collect_merges_scanned_methods_into_broma_ccnode(self) -> None:
        tmpdir = tempfile.mkdtemp()
        sdk = tempfile.mkdtemp()
        try:
            _write_empty_broma_dir(tmpdir)
            _write(
                os.path.join(tmpdir, "Cocos2d.bro"),
                "[[link(win)]] class cocos2d::CCObject {};"
                "[[link(win)]] class cocos2d::CCNode : cocos2d::CCObject {"
                "void setVisible(bool visible) = win 0x1;"
                "};",
            )
            _write_ccnode_header(
                sdk,
                """
public:
    GEODE_DLL void setID(std::string id);
    GEODE_DLL geode::ZStringView getID();
    GEODE_DLL CCNode* getChildByID(std::string_view id);
""",
            )

            root = collect_bindings_root(tmpdir, geode_sdk_path=sdk)
            ccnode = next(c for c in root.classes if c.qualified_name == "cocos2d::CCNode")
            names = [m.name for m in ccnode.methods]
            self.assertEqual(names.count("setID"), 1)
            self.assertIn("setVisible", names)
            self.assertIn("getID", names)
            self.assertIn("getChildByID", names)
            self.assertEqual(class_link_platforms(ccnode), {"win"})
            plan = collect_plan(root, "win")
            luau = types_text(emit_luau_types(root, "win", plan=plan))
            self.assertIn("function setID(self, arg1: string)", luau)
            self.assertIn("function getID(self): string", luau)
            self.assertIn("function getChildByID(self, arg1: string): CCNode?", luau)
        finally:
            shutil.rmtree(tmpdir)
            shutil.rmtree(sdk)

    def test_emits_setid_binding_and_luau_type(self) -> None:
        ccobject = Class(
            name="CCObject",
            namespace="cocos2d",
            attributes=["link(win, android, android32, android64, imac, m1, ios)"],
        )
        set_id = Method(
            name="setID",
            ret="void",
            args=[Arg("std::string", "id")],
            platforms=all_platforms("link"),
        )
        get_id = Method(
            name="getID",
            ret="geode::ZStringView",
            args=[],
            platforms=all_platforms("link"),
        )
        ccnode = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[set_id, get_id],
            attributes=["link(win, android, android32, android64, imac, m1, ios)"],
        )
        root = Root(classes=[ccobject, ccnode])
        objects = {
            "CCObject": ccobject,
            "CCNode": ccnode,
            "cocos2d::CCNode": ccnode,
        }

        cxx = _emit_class_file(
            ccnode,
            {"setID": [set_id], "getID": [get_id]},
            [],
            [],
            objects,
            set(),
            1,
            "win",
        )
        self.assertIn("self->setID(arg0);", cxx)

        plan = collect_plan(root, "win")
        luau = types_text(emit_luau_types(root, "win", plan=plan))
        self.assertIn("function setID(self, arg1: string)", luau)
        self.assertIn("function getID(self): string", luau)

    def test_getchildbyid_binding_uses_dynamic_push(self) -> None:
        ccobject = Class(
            name="CCObject",
            namespace="cocos2d",
            attributes=["link(win, android, android32, android64, imac, m1, ios)"],
        )
        get_child = Method(
            name="getChildByID",
            ret="CCNode*",
            args=[Arg("std::string_view", "id")],
            platforms=all_platforms("link"),
        )
        ccnode = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[get_child],
            attributes=["link(win, android, android32, android64, imac, m1, ios)"],
        )
        root = Root(classes=[ccobject, ccnode])
        objects = {
            "CCObject": ccobject,
            "CCNode": ccnode,
            "cocos2d::CCNode": ccnode,
        }

        cxx = _emit_class_file(
            ccnode,
            {"getChildByID": [get_child]},
            [],
            [],
            objects,
            set(),
            1,
            "win",
        )
        self.assertIn("self->getChildByID(arg0)", cxx)
        self.assertIn("luax::Usertype<cocos2d::CCNode>::pushBorrowedDynamic(L, result);", cxx)
        self.assertNotIn("Usertype<cocos2d::CCObject>::pushBorrowedDynamic(L, result);", cxx)

    def test_unsupported_scanned_method_is_skipped_by_filter(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        unsupported = Method(
            name="getFieldContainer",
            ret="geode::modifier::FieldContainer*",
            args=[Arg("char const*", "forClass")],
        )
        ccnode = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[unsupported],
            attributes=["link(win)"],
        )

        grouped, skipped = group_supported(
            ccnode,
            {"CCObject": ccobject, "CCNode": ccnode},
            "win",
        )
        self.assertEqual(grouped, {})
        self.assertEqual(
            skipped[0][1],
            "unsupported-return:geode::modifier::FieldContainer*",
        )

    def test_anchor_enum_scan_supports_ccnode_anchor_args(self) -> None:
        from luau_codegen.parse.geode_sdk import (  # type: ignore[import-unresolved]
            scan_geode_enums,
        )

        tmpdir = tempfile.mkdtemp()
        try:
            include_dir = os.path.join(tmpdir, "loader", "include", "Geode")
            ui_dir = os.path.join(include_dir, "ui")
            _write(os.path.join(include_dir, "UI.hpp"), '#include "ui/Layout.hpp"\n')
            _write(
                os.path.join(ui_dir, "Layout.hpp"),
                "namespace geode { enum class Anchor { Center, Top }; }",
            )

            enums = scan_geode_enums(tmpdir)
            ctx = register_geode_enums({name: info.cxx_name for name, info in enums.items()})
            info = classify_arg("geode::Anchor", {}, ctx=ctx)
            self.assertIsNotNone(info)
            assert info is not None
            self.assertEqual(info.kind, "enum")
            self.assertEqual(info.cxx_type, "geode::Anchor")
        finally:
            shutil.rmtree(tmpdir)
