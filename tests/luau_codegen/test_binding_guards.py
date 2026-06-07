from __future__ import annotations

import os
import re
import unittest

_REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

_WEB_BINDING = "src/lua/bindings/geode/GeodeWebBinding.cpp"
_FS_BINDING = "src/lua/bindings/geode/GeodeFsBinding.cpp"
_TASK_SCHEDULER = "src/lua/bindings/task/TaskScheduler.cpp"
_TASK_BINDING = "src/lua/bindings/task/TaskBinding.cpp"
_CONFIG_HEADER = "src/lua/Config.hpp"


def _read_repo_file(rel_path: str) -> str:
    path = os.path.join(_REPO_ROOT, rel_path)
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def _function_body(source: str, name: str) -> str:
    marker = f"int {name}(lua_State* L) {{"
    start = source.find(marker)
    assert start != -1, f"missing {name} implementation"
    next_fn = source.find("\n    int ", start + len(marker))
    if next_fn == -1:
        next_fn = len(source)
    return source[start:next_fn]


class BindingGuardTests(unittest.TestCase):
    def test_web_response_methods_enforce_size_cap(self) -> None:
        source = _read_repo_file(_WEB_BINDING)
        for method in (
            "responseText",
            "responseBytes",
            "responseSaveTo",
            "responseJson",
        ):
            with self.subTest(method=method):
                body = _function_body(source, method)
                self.assertIn(
                    "responseDataWithinLimit",
                    body,
                    f"{method} must enforce kMaxWebResponseBytes",
                )

    def test_web_response_cap_constant_is_declared(self) -> None:
        config = _read_repo_file(_CONFIG_HEADER)
        self.assertIn("kMaxWebResponseBytes", config)

    def test_fs_write_enforces_size_cap(self) -> None:
        source = _read_repo_file(_FS_BINDING)
        body = _function_body(source, "fsWrite")
        self.assertIn(
            "kMaxFsWriteBytes",
            body,
            "fsWrite must enforce kMaxFsWriteBytes",
        )

    def test_fs_write_cap_constant_is_declared(self) -> None:
        config = _read_repo_file(_CONFIG_HEADER)
        self.assertIn("kMaxFsWriteBytes", config)

    def test_arm_task_tick_logs_director_and_scheduler_failures(self) -> None:
        source = _read_repo_file(_TASK_SCHEDULER)
        host_guard = "#if !defined(LUAUAPI_HOST_TESTS)"
        start = source.find(host_guard)
        self.assertNotEqual(start, -1, "missing host-test guard in TaskScheduler.cpp")
        end = source.find("#else", start)
        self.assertNotEqual(end, -1, "missing host-test else branch")
        production = source[start:end]
        self.assertIn("CCDirector unavailable", production)
        self.assertIn("cocos2d scheduler unavailable", production)

    def test_arm_task_tick_returns_bool(self) -> None:
        source = _read_repo_file(_TASK_SCHEDULER)
        self.assertIn("bool armTaskTick()", source)

    def test_host_tests_stub_arm_task_tick(self) -> None:
        source = _read_repo_file(_TASK_SCHEDULER)
        else_start = source.find("#else")
        host_branch = source[else_start:]
        self.assertIn("bool armTaskTick()", host_branch)
        self.assertIn("return false;", host_branch)
        self.assertIn("void disarmTaskTick() {}", host_branch)

    def test_task_schedulers_rearm_on_each_call(self) -> None:
        source = _read_repo_file(_TASK_BINDING)
        for fn in ("taskDelay", "taskEvery", "taskDefer"):
            with self.subTest(fn=fn):
                body = _function_body(source, fn)
                self.assertIn(
                    "ensureTaskTickArmed",
                    body,
                    f"{fn} must lazily re-arm the scheduler tick",
                )


class FreeFnManifestSyncTests(unittest.TestCase):
    def test_scanner_and_emitter_share_one_manifest(self) -> None:
        from luau_codegen.model.free_fn_sources import (
            FREE_FUNCTION_SOURCES,
            free_function_includes,
        )
        from luau_codegen.parse import geode_sdk

        self.assertIs(geode_sdk._FUNCTION_SOURCES, FREE_FUNCTION_SOURCES)

        includes = free_function_includes()
        self.assertEqual(
            len(includes),
            len(FREE_FUNCTION_SOURCES),
            "one include per scanned free-function header",
        )
        for (rel, _ns, _names), inc in zip(FREE_FUNCTION_SOURCES, includes):
            self.assertEqual(inc, f"Geode/{rel}")

    def test_generated_binding_includes_match_manifest(self) -> None:
        from luau_codegen.emit.bindings.free_functions import emit_free_functions_file
        from luau_codegen.model.free_fn_sources import free_function_includes

        emitted = emit_free_functions_file([], {})
        for header in free_function_includes():
            self.assertIn(
                f"#include <{header}>",
                emitted,
                f"generated free-function file must #include {header}",
            )


if __name__ == "__main__":
    unittest.main()
