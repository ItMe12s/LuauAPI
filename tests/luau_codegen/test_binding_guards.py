from __future__ import annotations

import os
import re
import unittest

_REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

_WEB_BINDING = "src/lua/bindings/geode/GeodeWebBinding.cpp"
_TASK_SCHEDULER = "src/lua/bindings/task/TaskScheduler.cpp"
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
        for method in ("responseText", "responseBytes", "responseSaveTo"):
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

    def test_host_tests_stub_arm_task_tick(self) -> None:
        source = _read_repo_file(_TASK_SCHEDULER)
        else_start = source.find("#else")
        host_branch = source[else_start:]
        self.assertIn("void armTaskTick() {}", host_branch)
        self.assertIn("void disarmTaskTick() {}", host_branch)


if __name__ == "__main__":
    unittest.main()
