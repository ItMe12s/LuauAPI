from __future__ import annotations

import os
import shutil
import tempfile
import unittest
from unittest import mock
from helpers import (
    Class,  # type: ignore[import-unresolved]
    class_link_platforms,  # type: ignore[import-unresolved]
    supported,  # type: ignore[import-unresolved]
)


class M1ScannerWarningTests(unittest.TestCase):
    def test_bad_header_emits_warning(self) -> None:
        from luau_codegen.parse.geode_sdk import (  # type: ignore[import-unresolved]
            scan_geode_sdk,
            take_scan_warnings,
        )

        tmpdir = tempfile.mkdtemp()
        try:
            ui_dir = os.path.join(tmpdir, "loader", "include", "Geode", "ui")
            os.makedirs(ui_dir)
            include_dir = os.path.dirname(ui_dir)
            with open(os.path.join(include_dir, "UI.hpp"), "w", encoding="utf-8") as f:
                f.write('#include "ui/Bad.hpp"\n')
            with open(os.path.join(ui_dir, "Bad.hpp"), "w") as f:
                f.write("this is not valid C++ {{{{ [[[ ;;;")
            take_scan_warnings()
            with self.assertLogs("luau_codegen.geode_sdk", level="WARNING") as logs:
                with mock.patch(
                    "luau_codegen.parse.geode_sdk._scan_header",
                    side_effect=ValueError("bad"),
                ):
                    scan_geode_sdk(tmpdir)
            recorded = take_scan_warnings()
            self.assertTrue(any("[luauapi] failed to scan" in m for m in recorded))
            self.assertTrue(any("[luauapi] failed to scan" in line for line in logs.output))
        finally:
            shutil.rmtree(tmpdir)

    def test_scanned_ui_class_links_all_platforms(self) -> None:
        from luau_codegen.parse.geode_sdk import scan_geode_sdk  # type: ignore[import-unresolved]

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
            self.assertEqual(
                class_link_platforms(classes[0]),
                {"win", "android", "android32", "android64", "imac", "m1", "ios"},
            )

            ccobject = Class(name="CCObject", namespace="cocos2d")
            objects = {"CCObject": ccobject, "GoodUI": classes[0]}
            ok_win, _ = supported(classes[0], classes[0].methods[0], objects, "win")
            ok_android, reason = supported(classes[0], classes[0].methods[0], objects, "android64")
            self.assertTrue(ok_win)
            self.assertTrue(ok_android)
            self.assertEqual(reason, "")
        finally:
            shutil.rmtree(tmpdir)


class GeodeScannerFixtureTests(unittest.TestCase):
    def test_nested_namespace_class_scanned(self) -> None:
        from luau_codegen.parse.geode_sdk import scan_geode_sdk  # type: ignore[import-unresolved]

        tmpdir = tempfile.mkdtemp()
        try:
            ui_dir = os.path.join(tmpdir, "loader", "include", "Geode", "ui")
            os.makedirs(ui_dir)
            include_dir = os.path.dirname(ui_dir)
            with open(os.path.join(include_dir, "UI.hpp"), "w", encoding="utf-8") as f:
                f.write('#include "ui/Nested.hpp"\n')
            with open(os.path.join(ui_dir, "Nested.hpp"), "w", encoding="utf-8") as f:
                f.write(
                    "namespace geode::ui { "
                    "class GEODE_DLL NestedUI : public cocos2d::CCObject { "
                    "public: void open(); "
                    "}; "
                    "}"
                )
            classes = scan_geode_sdk(tmpdir)
            self.assertEqual(len(classes), 1)
            self.assertEqual(classes[0].namespace, "geode::ui")
            self.assertEqual(classes[0].name, "NestedUI")
        finally:
            shutil.rmtree(tmpdir)

    def test_template_specialization_class_skipped(self) -> None:
        from luau_codegen.parse.geode_sdk import scan_geode_sdk  # type: ignore[import-unresolved]

        tmpdir = tempfile.mkdtemp()
        try:
            ui_dir = os.path.join(tmpdir, "loader", "include", "Geode", "ui")
            os.makedirs(ui_dir)
            include_dir = os.path.dirname(ui_dir)
            with open(os.path.join(include_dir, "UI.hpp"), "w", encoding="utf-8") as f:
                f.write('#include "ui/Template.hpp"\n')
            with open(os.path.join(ui_dir, "Template.hpp"), "w", encoding="utf-8") as f:
                f.write(
                    "namespace geode { "
                    "template<typename T> "
                    "class GEODE_DLL Box { public: void reset(); }; "
                    "template<> "
                    "class GEODE_DLL Box<int> : public cocos2d::CCObject { "
                    "public: void reset(); "
                    "}; "
                    "class GEODE_DLL PlainUI : public cocos2d::CCObject { "
                    "public: void open(); "
                    "}; "
                    "}"
                )
            classes = scan_geode_sdk(tmpdir)
            names = [c.name for c in classes]
            self.assertEqual(names, ["PlainUI"])
        finally:
            shutil.rmtree(tmpdir)

    def test_method_with_attribute_and_brace_in_string_literal(self) -> None:
        from luau_codegen.parse.geode_sdk import scan_geode_sdk  # type: ignore[import-unresolved]

        tmpdir = tempfile.mkdtemp()
        try:
            ui_dir = os.path.join(tmpdir, "loader", "include", "Geode", "ui")
            os.makedirs(ui_dir)
            include_dir = os.path.dirname(ui_dir)
            with open(os.path.join(include_dir, "UI.hpp"), "w", encoding="utf-8") as f:
                f.write('#include "ui/Attr.hpp"\n')
            with open(os.path.join(ui_dir, "Attr.hpp"), "w", encoding="utf-8") as f:
                f.write(
                    "namespace geode { "
                    "class GEODE_DLL AttrUI : public cocos2d::CCObject { "
                    "public: "
                    "[[nodiscard]] void show(char const* label); "
                    'void hide() { log("}"); } '
                    "}; "
                    "}"
                )
            classes = scan_geode_sdk(tmpdir)
            self.assertEqual(len(classes), 1)
            names = [m.name for m in classes[0].methods]
            self.assertIn("show", names)
            self.assertIn("hide", names)
        finally:
            shutil.rmtree(tmpdir)


class GeodeEnumParserTests(unittest.TestCase):
    def test_parse_enum_members_unit(self) -> None:
        from luau_codegen.model.geode_enums import (  # type: ignore[import-unresolved]
            EnumMember,
            parse_enum_members,
        )

        members, warning = parse_enum_members(
            "Default = 0, Main, Editor = 2, Saved = 3, SearchResult = 4"
        )
        self.assertIsNone(warning)
        self.assertEqual(
            members,
            (
                EnumMember("Default", 0),
                EnumMember("Main", 1),
                EnumMember("Editor", 2),
                EnumMember("Saved", 3),
                EnumMember("SearchResult", 4),
            ),
        )

    def test_parse_gj_level_type_members(self) -> None:
        from luau_codegen.model.geode_enums import EnumMember  # type: ignore[import-unresolved]
        from luau_codegen.parse.geode_sdk import parse_geode_enums  # type: ignore[import-unresolved]

        enums = parse_geode_enums(
            """
            enum class GJLevelType {
                Default = 0,
                Main = 1,
                Editor = 2,
                Saved = 3,
                SearchResult = 4
            };
            """
        )
        self.assertIn("GJLevelType", enums)
        info = enums["GJLevelType"]
        self.assertEqual(info.cxx_name, "GJLevelType")
        self.assertEqual(
            info.members,
            (
                EnumMember("Default", 0),
                EnumMember("Main", 1),
                EnumMember("Editor", 2),
                EnumMember("Saved", 3),
                EnumMember("SearchResult", 4),
            ),
        )

    def test_parse_enum_members_implicit_increment(self) -> None:
        from luau_codegen.model.geode_enums import EnumMember  # type: ignore[import-unresolved]
        from luau_codegen.parse.geode_sdk import parse_geode_enums  # type: ignore[import-unresolved]

        enums = parse_geode_enums(
            "enum class UpdateResponse { Unknown, UpToDate, GameVerOutOfDate, UpdateSuccess, };"
        )
        self.assertEqual(
            enums["UpdateResponse"].members,
            (
                EnumMember("Unknown", 0),
                EnumMember("UpToDate", 1),
                EnumMember("GameVerOutOfDate", 2),
                EnumMember("UpdateSuccess", 3),
            ),
        )

    def test_parse_enum_members_negative_and_hex(self) -> None:
        from luau_codegen.model.geode_enums import EnumMember  # type: ignore[import-unresolved]
        from luau_codegen.parse.geode_sdk import parse_geode_enums  # type: ignore[import-unresolved]

        enums = parse_geode_enums(
            """
            enum class Sample {
                Negative = -2,
                Hex = 0xA,
                Next
            };
            """
        )
        self.assertEqual(
            enums["Sample"].members,
            (
                EnumMember("Negative", -2),
                EnumMember("Hex", 10),
                EnumMember("Next", 11),
            ),
        )

    def test_parse_enum_members_binary_literals(self) -> None:
        from luau_codegen.model.geode_enums import EnumMember  # type: ignore[import-unresolved]
        from luau_codegen.parse.geode_sdk import parse_geode_enums  # type: ignore[import-unresolved]

        enums = parse_geode_enums("enum class Flags { A = 0b0001, B = 0b0010, };")

        self.assertEqual(
            enums["Flags"].members,
            (EnumMember("A", 1), EnumMember("B", 2)),
        )

    def test_parse_enum_members_skips_unsupported_expression(self) -> None:
        from luau_codegen.parse.geode_sdk import (  # type: ignore[import-unresolved]
            parse_geode_enums,
            take_scan_warnings,
        )

        take_scan_warnings()
        enums = parse_geode_enums(
            "enum class Bad { A = (1 << 2), B = 3 };",
            source="Bad.hpp",
        )
        self.assertEqual(enums["Bad"].members, ())
        warnings = take_scan_warnings()
        self.assertTrue(any("unsupported enum value expression" in w for w in warnings))

    def test_scan_geode_enums_reads_enums_hpp(self) -> None:
        from luau_codegen.model.geode_enums import EnumMember  # type: ignore[import-unresolved]
        from luau_codegen.parse.geode_sdk import scan_geode_enums  # type: ignore[import-unresolved]

        tmpdir = tempfile.mkdtemp()
        try:
            include_dir = os.path.join(tmpdir, "loader", "include", "Geode")
            os.makedirs(include_dir)
            with open(os.path.join(include_dir, "Enums.hpp"), "w", encoding="utf-8") as f:
                f.write("enum class GJLevelType {\n    Saved = 3,\n    SearchResult = 4\n};\n")
            enums = scan_geode_enums(tmpdir)
            self.assertIn("GJLevelType", enums)
            self.assertEqual(
                enums["GJLevelType"].members,
                (
                    EnumMember("Saved", 3),
                    EnumMember("SearchResult", 4),
                ),
            )
        finally:
            shutil.rmtree(tmpdir)

    def test_scan_geode_enums_reads_bindings_repo_enums_hpp(self) -> None:
        from luau_codegen.model.geode_enums import EnumMember  # type: ignore[import-unresolved]
        from luau_codegen.parse.geode_sdk import scan_geode_enums  # type: ignore[import-unresolved]

        repo = tempfile.mkdtemp()
        sdk = tempfile.mkdtemp()
        try:
            bindings_dir = os.path.join(repo, "bindings", "2.2074")
            include_dir = os.path.join(repo, "bindings", "include", "Geode")
            os.makedirs(bindings_dir)
            os.makedirs(include_dir)
            with open(os.path.join(include_dir, "Enums.hpp"), "w", encoding="utf-8") as f:
                f.write("enum class GJLevelType { Saved = 3, SearchResult = 4 };\n")

            enums = scan_geode_enums(sdk, bindings_dir=bindings_dir)

            self.assertEqual(
                enums["GJLevelType"].members,
                (EnumMember("Saved", 3), EnumMember("SearchResult", 4)),
            )
        finally:
            shutil.rmtree(repo)
            shutil.rmtree(sdk)

    def test_parse_enum_with_base_type(self) -> None:
        from luau_codegen.model.geode_enums import EnumMember  # type: ignore[import-unresolved]
        from luau_codegen.parse.geode_sdk import parse_geode_enums  # type: ignore[import-unresolved]

        enums = parse_geode_enums(
            "namespace geode { enum class ScaleButtonType : short { X = 0, Y = 1, XY = 2, }; }"
        )
        self.assertEqual(enums["ScaleButtonType"].cxx_name, "geode::ScaleButtonType")
        self.assertEqual(
            enums["ScaleButtonType"].members,
            (
                EnumMember("X", 0),
                EnumMember("Y", 1),
                EnumMember("XY", 2),
            ),
        )

    def test_parse_enums_hpp_ignores_later_namespace(self) -> None:
        from luau_codegen.model.geode_enums import EnumMember  # type: ignore[import-unresolved]
        from luau_codegen.parse.geode_sdk import parse_geode_enums  # type: ignore[import-unresolved]

        enums = parse_geode_enums(
            """
            enum class GJLevelType { Saved = 3, SearchResult = 4 };
            namespace GameVar { constexpr auto FollowPlayer = "0001"; }
            """
        )
        self.assertEqual(enums["GJLevelType"].cxx_name, "GJLevelType")
        self.assertEqual(
            enums["GJLevelType"].members,
            (
                EnumMember("Saved", 3),
                EnumMember("SearchResult", 4),
            ),
        )

    def test_scan_geode_enums_merges_ui_and_enums_hpp(self) -> None:
        from luau_codegen.model.geode_enums import EnumMember  # type: ignore[import-unresolved]
        from luau_codegen.parse.geode_sdk import scan_geode_enums  # type: ignore[import-unresolved]

        tmpdir = tempfile.mkdtemp()
        try:
            include_dir = os.path.join(tmpdir, "loader", "include", "Geode")
            ui_dir = os.path.join(include_dir, "ui")
            os.makedirs(ui_dir)
            with open(os.path.join(include_dir, "UI.hpp"), "w", encoding="utf-8") as f:
                f.write('#include "ui/Layout.hpp"\n')
            with open(os.path.join(ui_dir, "Layout.hpp"), "w", encoding="utf-8") as f:
                f.write("namespace geode { enum class Anchor { Center = 0, Top = 1 }; }")
            with open(os.path.join(include_dir, "Enums.hpp"), "w", encoding="utf-8") as f:
                f.write("enum class GJLevelType { Saved = 3, SearchResult = 4 };\n")
            enums = scan_geode_enums(tmpdir)
            self.assertEqual(enums["Anchor"].cxx_name, "geode::Anchor")
            self.assertEqual(
                enums["Anchor"].members,
                (EnumMember("Center", 0), EnumMember("Top", 1)),
            )
            self.assertEqual(enums["GJLevelType"].cxx_name, "GJLevelType")
            self.assertEqual(
                enums["GJLevelType"].members,
                (EnumMember("Saved", 3), EnumMember("SearchResult", 4)),
            )
        finally:
            shutil.rmtree(tmpdir)


class ExtraScanScopeTests(unittest.TestCase):
    def test_function_source_manifest_keeps_expected_headers(self) -> None:
        from luau_codegen.model.free_fn_sources import FREE_FUNCTION_SOURCES
        from luau_codegen.parse import geode_sdk  # type: ignore[import-unresolved]

        rels = {rel for rel, _, _ in FREE_FUNCTION_SOURCES}
        self.assertGreaterEqual(
            rels,
            {
                "utils/general.hpp",
                "ui/Popup.hpp",
                "ui/GeodeUI.hpp",
                "utils/string.hpp",
                "utils/random.hpp",
                "utils/cocos.hpp",
            },
        )

    def test_multiline_free_function_decl_scanned(self) -> None:
        from luau_codegen.parse.geode_sdk import scan_geode_functions  # type: ignore[import-unresolved]

        tmpdir = tempfile.mkdtemp()
        try:
            utils_dir = os.path.join(tmpdir, "loader", "include", "Geode", "utils")
            os.makedirs(utils_dir)
            with open(os.path.join(utils_dir, "general.hpp"), "w", encoding="utf-8") as f:
                f.write(
                    "namespace geode::utils::game {\n"
                    "GEODE_DLL std::string\n"
                    "normalizePath(\n"
                    "    char const* input,\n"
                    "    int flags\n"
                    ");\n"
                    "}\n"
                )
            fns = {f.name: f for f in scan_geode_functions(tmpdir)}
            self.assertIn("normalizePath", fns)
            self.assertEqual(fns["normalizePath"].namespace, "geode::utils::game")
            self.assertEqual(len(fns["normalizePath"].args), 2)
        finally:
            shutil.rmtree(tmpdir)

    def test_utils_namespace_free_functions_scanned(self) -> None:
        from luau_codegen.parse.geode_sdk import scan_geode_functions  # type: ignore[import-unresolved]

        tmpdir = tempfile.mkdtemp()
        try:
            utils_dir = os.path.join(tmpdir, "loader", "include", "Geode", "utils")
            os.makedirs(utils_dir)
            with open(os.path.join(utils_dir, "random.hpp"), "w", encoding="utf-8") as f:
                f.write("namespace geode::utils::random { GEODE_DLL std::string generateUUID(); }")
            fns = {f.name: f for f in scan_geode_functions(tmpdir)}
            self.assertIn("generateUUID", fns)
            self.assertEqual(fns["generateUUID"].namespace, "geode::utils::random")
        finally:
            shutil.rmtree(tmpdir)
