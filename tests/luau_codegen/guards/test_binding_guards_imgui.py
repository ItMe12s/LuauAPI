from __future__ import annotations

import unittest

from binding_guard_support import (
    CMAKE_LISTS,
    IMGUI_BINDING,
    IMGUI_BINDING_TESTS,
    IMGUI_HEADLESS_CMAKE,
    IMGUI_HOST_STUB,
    IMGUI_STYLE,
    TEST_SOURCES_CMAKE,
    function_body,
    read_repo_file,
)


class ImGuiGuardTests(unittest.TestCase):
    def test_host_tests_use_imgui_host_stub(self) -> None:
        test_sources = read_repo_file(TEST_SOURCES_CMAKE)
        self.assertIn("tests/host/ImGuiHostStub.cpp", test_sources)

        cmake = read_repo_file(CMAKE_LISTS)
        self.assertNotIn("src/bindings/imgui/ImGuiHost.cpp", cmake)
        self.assertNotIn("src/bindings/imgui/ImGuiHost.cpp", test_sources)

        stub = read_repo_file(IMGUI_HOST_STUB)
        for fn in (
            "initImGuiHost",
            "shutdownImGuiHost",
            "imguiHostSetVisible",
            "imguiHostToggle",
        ):
            with self.subTest(fn=fn):
                self.assertIn(f"void {fn}", stub)
        self.assertIn("bool imguiHostIsVisible", stub)

    def test_on_draw_skips_imgui_cocos_init_in_host_tests(self) -> None:
        source = read_repo_file(IMGUI_BINDING)
        self.assertIn("#if !defined(LUAUAPI_HOST_TESTS)", source)
        self.assertIn("initImGuiHost();", source)

    def test_style_with_uses_raii_pop_guards(self) -> None:
        source = read_repo_file(IMGUI_STYLE)
        self.assertIn("ImGuiStyleVarPopGuard", source)
        self.assertIn("ImGuiStyleColorPopGuard", source)
        body = function_body(source, "imguiStyleWith")
        self.assertIn("callDrawClosure", body)

    def test_scoped_imgui_wrappers_use_end_guards(self) -> None:
        scoped_sources = (
            IMGUI_BINDING,
            "src/bindings/imgui/ImGuiPopupsTablesMenus.cpp",
            "src/bindings/imgui/ImGuiWidgetsLayout.cpp",
        )
        for source_path in scoped_sources:
            with self.subTest(source=source_path):
                source = read_repo_file(source_path)
                self.assertTrue(
                    "ImGuiEndGuard" in source or "ImGuiConditionalEndGuard" in source,
                    f"{source_path} must use scoped end guards",
                )

    def test_imgui_binding_tests_cover_headless_checks(self) -> None:
        source = read_repo_file(IMGUI_BINDING_TESTS)
        for needle in (
            "ImGuiTestHarness.hpp",
            "ImGuiContextGuard",
            "beginImGuiTestFrame",
            "endImGuiTestFrame",
            "ImGuiWindowFlags_NoTitleBar",
            "imgui.style.with",
            "secondWindowOk",
            "recoveredTabOk",
        ):
            with self.subTest(needle=needle):
                self.assertIn(needle, source)

    def test_cmake_links_headless_imgui_for_host_tests(self) -> None:
        cmake = read_repo_file(CMAKE_LISTS)
        test_sources = read_repo_file(TEST_SOURCES_CMAKE)
        headless = read_repo_file(IMGUI_HEADLESS_CMAKE)
        self.assertIn("cmake/ImGuiHeadless.cmake", cmake)
        self.assertIn("cmake/TestSources.cmake", cmake)
        self.assertIn("luauapi_imgui_headless", cmake)
        self.assertIn("imgui_impl_null.cpp", headless)
        self.assertIn("host/ImGuiTestHarness.hpp", read_repo_file(IMGUI_BINDING_TESTS))
        for source in (
            "src/bindings/imgui/ImGuiCore.cpp",
            "src/bindings/imgui/ImGuiStyleFonts.cpp",
            "src/bindings/imgui/ImGuiWidgetsLayout.cpp",
        ):
            with self.subTest(source=source):
                self.assertIn(source, test_sources)
