from __future__ import annotations

import unittest
from unittest import mock

from luau_codegen.model.platforms import (
    SCANNED_LINK_ATTR,
    SCANNED_LINK_PLATFORMS,
    STRICT_DIRECT_PLATFORMS,
    platform_value,
)
from luau_codegen.parse.geode_sdk import scan_geode_sdk


class ScannerDiagnosticsTests(unittest.TestCase):
    def test_scanner_writes_to_explicit_diagnostics_sink(self) -> None:
        diagnostics: list[str] = []
        with mock.patch(
            "luau_codegen.parse.geode_sdk._iter_ui_headers", return_value=[("bad", "Bad.hpp")]
        ):
            with mock.patch(
                "luau_codegen.parse.geode_sdk._scan_header", side_effect=ValueError("bad")
            ):
                scan_geode_sdk("sdk", diagnostics=diagnostics)

        self.assertEqual(diagnostics, ["[luauapi] failed to scan Bad.hpp: bad"])


class PlatformModelTests(unittest.TestCase):
    def test_platform_constants_and_fallbacks(self) -> None:
        self.assertEqual(STRICT_DIRECT_PLATFORMS, frozenset({"ios"}))
        self.assertEqual(SCANNED_LINK_ATTR, f"link({', '.join(SCANNED_LINK_PLATFORMS)})")
        self.assertEqual(platform_value({"m1": "m"}, "mac"), "m")
        self.assertEqual(platform_value({"android32": "a"}, "android"), "a")
