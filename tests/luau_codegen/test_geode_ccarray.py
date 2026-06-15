from __future__ import annotations

import os
import shutil
import tempfile
import unittest
from helpers import (
    Arg,  # type: ignore[import-unresolved]
    Class,  # type: ignore[import-unresolved]
    Method,  # type: ignore[import-unresolved]
    Root,  # type: ignore[import-unresolved]
    _emit_class_file,  # type: ignore[import-unresolved]
    all_platforms,  # type: ignore[import-unresolved]
    class_link_platforms,  # type: ignore[import-unresolved]
    collect_bindings_root,  # type: ignore[import-unresolved]
    collect_plan,  # type: ignore[import-unresolved]
    emit_luau_types,  # type: ignore[import-unresolved]
    types_text,  # type: ignore[import-unresolved]
)


def _write(path: str, text: str) -> None:
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        f.write(text)


def _write_empty_broma_dir(path: str) -> None:
    from luau_codegen.model.domain import BRO_FILES  # type: ignore[import-unresolved]

    os.makedirs(path, exist_ok=True)
    for name in BRO_FILES:
        _write(os.path.join(path, name), "")


def _write_ccarray_header(sdk_path: str, body: str) -> str:
    path = os.path.join(
        sdk_path,
        "loader",
        "include",
        "Geode",
        "cocos",
        "cocoa",
        "CCArray.h",
    )
    _write(
        path,
        f"NS_CC_BEGIN\nclass CC_DLL CCArray : public CCObject\n{{\n{body}\n}};\nNS_CC_END\n",
    )
    return path


class GeodeCCArrayScannerTests(unittest.TestCase):
    def test_scans_public_allowlisted_vanilla_methods_only(self) -> None:
        from luau_codegen.parse.geode_sdk import (  # type: ignore[import-unresolved]
            scan_geode_ccarray_additions,
        )

        tmpdir = tempfile.mkdtemp()
        try:
            _write_ccarray_header(
                tmpdir,
                """
public:
    unsigned int count() const;
    CCObject* objectAtIndex(unsigned int index);
    CCObject* firstObject();
    void addObject(CCObject* object);
    GEODE_FRIEND_MODIFY
    GEODE_DLL void addObjectNew(CCObject* object);
protected:
    unsigned int capacity() const;
private:
    void removeObject(CCObject* object);
""",
            )

            cls = scan_geode_ccarray_additions(tmpdir)
            self.assertIsNotNone(cls)
            assert cls is not None
            self.assertEqual(cls.qualified_name, "cocos2d::CCArray")
            self.assertEqual(cls.attributes, [])
            self.assertEqual(
                set(cls.methods[0].platforms),
                {"win", "android", "android32", "android64", "imac", "m1", "ios"},
            )

            names = [m.name for m in cls.methods]
            self.assertEqual(names, ["count", "objectAtIndex", "firstObject"])

            count = next(m for m in cls.methods if m.name == "count")
            self.assertEqual(count.ret, "unsigned int")
            self.assertEqual(count.args, [])

            object_at_index = next(m for m in cls.methods if m.name == "objectAtIndex")
            self.assertEqual(object_at_index.ret, "CCObject*")
            self.assertEqual([a.type for a in object_at_index.args], ["unsigned int"])
        finally:
            shutil.rmtree(tmpdir)

    def test_missing_ccarray_header_is_nonfatal(self) -> None:
        from luau_codegen.parse.geode_sdk import (  # type: ignore[import-unresolved]
            scan_geode_ccarray_additions,
        )

        tmpdir = tempfile.mkdtemp()
        try:
            self.assertIsNone(scan_geode_ccarray_additions(tmpdir))
        finally:
            shutil.rmtree(tmpdir)

    def test_collect_merges_scanned_methods_into_broma_ccarray(self) -> None:
        tmpdir = tempfile.mkdtemp()
        sdk = tempfile.mkdtemp()
        try:
            _write_empty_broma_dir(tmpdir)
            _write(
                os.path.join(tmpdir, "Cocos2d.bro"),
                "[[link(win, m1, ios, android32, android64)]] class cocos2d::CCObject {"
                "void retain() = win 0x1, m1 0x2, ios 0x3, android32 0x4, android64 0x5; };"
                "[[link(win)]] class cocos2d::CCArray : cocos2d::CCObject {"
                "void addObjectNew(CCObject* object) = win 0x1;"
                "};",
            )
            _write_ccarray_header(
                sdk,
                """
public:
    unsigned int count() const;
    CCObject* objectAtIndex(unsigned int index);
    void addObject(CCObject* object);
""",
            )

            root = collect_bindings_root(tmpdir, geode_sdk_path=sdk)
            ccarray = next(c for c in root.classes if c.qualified_name == "cocos2d::CCArray")
            names = [m.name for m in ccarray.methods]
            self.assertEqual(names.count("count"), 1)
            self.assertEqual(names.count("objectAtIndex"), 1)
            self.assertIn("addObjectNew", names)
            self.assertNotIn("addObject", names)
            self.assertEqual(class_link_platforms(ccarray), {"win"})
            plan = collect_plan(root, "win")
            luau = types_text(emit_luau_types(root, "win", plan=plan))
            self.assertIn("function count(self): number", luau)
            self.assertIn("function objectAtIndex(self, arg1: number): CCObject?", luau)
        finally:
            shutil.rmtree(tmpdir)
            shutil.rmtree(sdk)

    def test_emits_count_and_object_at_index_binding_and_luau_type(self) -> None:
        retain = Method(
            name="retain",
            ret="void",
            args=[],
            platforms=all_platforms("link"),
        )
        ccobject = Class(
            name="CCObject",
            namespace="cocos2d",
            methods=[retain],
            attributes=["link(win, android, android32, android64, imac, m1, ios)"],
        )
        count = Method(
            name="count",
            ret="unsigned int",
            args=[],
            platforms=all_platforms("link"),
        )
        object_at_index = Method(
            name="objectAtIndex",
            ret="CCObject*",
            args=[Arg("unsigned int", "index")],
            platforms=all_platforms("link"),
        )
        ccarray = Class(
            name="CCArray",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[count, object_at_index],
            attributes=["link(win, android, android32, android64, imac, m1, ios)"],
        )
        root = Root(classes=[ccobject, ccarray])
        objects = {
            "CCObject": ccobject,
            "CCArray": ccarray,
            "cocos2d::CCArray": ccarray,
        }

        cxx = _emit_class_file(
            ccarray,
            {"count": [count], "objectAtIndex": [object_at_index]},
            [],
            [],
            objects,
            set(),
            1,
            "win",
        )
        self.assertIn("auto result = self->count();", cxx)
        self.assertIn("auto result = self->objectAtIndex(arg0);", cxx)
        self.assertIn("luax::Usertype<cocos2d::CCObject>::pushBorrowedDynamic(L, result);", cxx)
        self.assertNotIn("luax::Usertype<cocos2d::CCObject>::pushBorrowed(L, result);", cxx)

        plan = collect_plan(root, "win")
        luau = types_text(emit_luau_types(root, "win", plan=plan))
        self.assertIn("function count(self): number", luau)
        self.assertIn("function objectAtIndex(self, arg1: number): CCObject?", luau)
