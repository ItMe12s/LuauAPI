from __future__ import annotations

import os
import shutil
import tempfile
import unittest
import warnings
from unittest import mock
from helpers import (
    Class,  # type: ignore[import-unresolved]
    class_link_platforms,  # type: ignore[import-unresolved]
    supported,  # type: ignore[import-unresolved]
)


class M1ScannerWarningTests(unittest.TestCase):
    def test_bad_header_emits_warning(self) -> None:
        from luau_codegen.parse.geode_sdk import scan_geode_sdk  # type: ignore[import-unresolved]

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
                    "luau_codegen.parse.geode_sdk._scan_header",
                    side_effect=ValueError("bad"),
                ):
                    scan_geode_sdk(tmpdir)
            warned = any("[luauapi] failed to scan" in str(x.message) for x in w)
            self.assertTrue(warned)
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


class ExtraScanScopeTests(unittest.TestCase):
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
