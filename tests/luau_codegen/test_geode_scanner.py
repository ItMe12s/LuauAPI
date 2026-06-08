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


class ExtraScanScopeTests(unittest.TestCase):
    def test_function_source_manifest_keeps_expected_headers(self) -> None:
        from luau_codegen.parse import geode_sdk  # type: ignore[import-unresolved]

        rels = {rel for rel, _, _ in geode_sdk._FUNCTION_SOURCES}
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
