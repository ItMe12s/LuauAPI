from __future__ import annotations

import os
import re
import unittest

from luau_codegen.emit.luau_types.manual_fields import (  # type: ignore[import-unresolved]
    MANUAL_FREE_FN_FIELDS,
)
from luau_codegen.model.free_fn_sources import FREE_FUNCTION_SOURCES  # type: ignore[import-unresolved]

_REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

_WEB_BINDING = "src/lua/bindings/geode/GeodeWebBinding.cpp"
_WEB_BINDING_SOURCES = (
    "src/lua/bindings/geode/GeodeWebBinding.cpp",
    "src/lua/bindings/geode/GeodeWebOptions.cpp",
    "src/lua/bindings/geode/GeodeWebRequest.cpp",
    "src/lua/bindings/geode/GeodeWebResponse.cpp",
    "src/lua/bindings/geode/GeodeWebMultipart.cpp",
    "src/lua/bindings/geode/GeodeWebListeners.cpp",
)
_WEB_INTERNAL = "src/lua/bindings/geode/WebInternal.hpp"
_FS_BINDING = "src/lua/bindings/geode/GeodeFsBinding.cpp"
_COCOS_BINDING = "src/lua/bindings/geode/GeodeCocosBinding.cpp"
_TASK_SCHEDULER = "src/lua/bindings/task/TaskScheduler.cpp"
_TASK_BINDING = "src/lua/bindings/task/TaskBinding.cpp"
_IMGUI_BINDING = "src/lua/bindings/imgui/ImGuiBinding.cpp"
_LUA_SELECTOR = "src/lua/bindings/framework/LuaSelectorHandler.cpp"
_LUA_MENU = "src/lua/bindings/framework/LuaMenuHandler.cpp"
_LUA_DELEGATE = "src/lua/bindings/framework/LuaDelegate.cpp"
_LUA_CALLBACK = "src/lua/bindings/framework/LuaCallback.hpp"
_GEODE_MOD = "src/lua/bindings/geode/GeodeModBinding.cpp"
_GEODE_PERMISSION = "src/lua/bindings/geode/GeodePermissionBinding.cpp"
_CONFIG_HEADER = "src/lua/Config.hpp"

_COCOS_GENERATED_OWNERS = frozenset(
    {
        "getObjectName",
        "handleTouchPriority",
        "handleTouchPriorityWith",
    }
)

_WEB_LISTENER_REGISTRARS = (
    "webOnRequestIntercept",
    "webOnRequestInterceptFor",
    "webOnRequestInterceptById",
    "webOnResponse",
    "webOnResponseFor",
    "webOnResponseById",
)

_REQUEST_BODY_METHODS = (
    "requestBody",
    "requestBodyString",
    "requestBodyJson",
    "requestBodyMultipart",
    "multipartParam",
    "multipartFile",
    "multipartFileFrom",
    "multipartGetBody",
)

_REQUEST_BODY_CAP_CHECKS = (
    "requestBodyWithinLimit",
    "requestJsonBodyWithinLimit",
)


def _web_binding_source() -> str:
    return "\n".join(_read_repo_file(path) for path in _WEB_BINDING_SOURCES)


def _read_repo_file(rel_path: str) -> str:
    path = os.path.join(_REPO_ROOT, rel_path)
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def _function_body(source: str, name: str, *, ret: str = "int") -> str:
    marker = f"{ret} {name}("
    start = source.find(marker)
    assert start != -1, f"missing {ret} {name} implementation"
    rest = source[start + len(marker) :]
    next_fn = re.search(r"\n    (?:int|void|bool|std::|geode::)", rest)
    end = start + len(marker) + next_fn.start() if next_fn else len(source)
    return source[start:end]


def _inline_function_body(source: str, signature: str) -> str:
    start = source.find(signature)
    assert start != -1, f"missing inline function {signature!r}"
    next_inline = source.find("\n    inline ", start + len(signature))
    next_namespace = source.find("\n} // namespace", start + len(signature))
    candidates = [pos for pos in (next_inline, next_namespace) if pos != -1]
    end = min(candidates) if candidates else len(source)
    return source[start:end]


def _registered_cocos_fields() -> set[str]:
    source = _read_repo_file(_COCOS_BINDING)
    marker = 'getOrCreateTable(L, "geode.cocos")'
    start = source.find(marker)
    assert start != -1, "missing geode.cocos registration block"
    end = source.find("return geode::Ok();", start)
    body = source[start:end]
    names = set(re.findall(r'setTableCFunction\(L,\s*[^,]+,\s*"([^"]+)"', body))
    names.update(re.findall(r'lua_setfield\(L,\s*[^,]+,\s*"([^"]+)"\)', body))
    return names


class BindingGuardTests(unittest.TestCase):
    def test_web_response_methods_enforce_size_cap(self) -> None:
        source = _web_binding_source()
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

    def test_push_response_enforces_size_cap_before_boxing(self) -> None:
        source = _read_repo_file(_WEB_INTERNAL)
        body = _inline_function_body(source, "inline void pushResponse(lua_State* L")
        self.assertIn(
            "kMaxWebResponseBytes",
            body,
            "pushResponse must reject oversized bodies before boxing userdata",
        )

    def test_web_request_body_cap_constant_is_declared(self) -> None:
        config = _read_repo_file(_CONFIG_HEADER)
        self.assertIn("kMaxWebRequestBytes", config)

    def test_apply_options_enforces_request_body_cap(self) -> None:
        source = _web_binding_source()
        body = _function_body(source, "applyOptions", ret="void")
        self.assertIn(
            "requestBodyWithinLimit",
            body,
            "applyOptions must enforce kMaxWebRequestBytes for option bodies",
        )

    def test_request_body_methods_enforce_size_cap(self) -> None:
        source = _web_binding_source()
        for method in _REQUEST_BODY_METHODS:
            with self.subTest(method=method):
                body = _function_body(source, method)
                self.assertTrue(
                    any(check in body for check in _REQUEST_BODY_CAP_CHECKS),
                    f"{method} must enforce kMaxWebRequestBytes",
                )

    def test_async_request_callback_rejects_oversized_response(self) -> None:
        source = _web_binding_source()
        body = _function_body(source, "startRequest", ret="std::shared_ptr<WebTask>")
        self.assertIn(
            "responseDataWithinLimit",
            body,
            "async request callbacks must reject oversized responses before pushResponse",
        )

    def test_response_listener_rejects_oversized_response(self) -> None:
        source = _web_binding_source()
        body = _function_body(source, "invokeResponseEventNow", ret="bool")
        self.assertIn(
            "responseDataWithinLimit",
            body,
            "response listeners must reject oversized responses before pushResponse",
        )

    def test_geode_web_binding_does_not_use_luaL_register(self) -> None:
        source = _web_binding_source()
        self.assertNotIn(
            "luaL_register",
            source,
            "GeodeWebBinding must register metatable methods without luaL_register",
        )

    def test_web_internal_has_no_dead_checkudata_null_checks(self) -> None:
        source = _read_repo_file(_WEB_INTERNAL)
        self.assertNotIn(
            "if (!box) luaL_error",
            source,
            "luaL_checkudata never returns null; redundant checks are dead code",
        )

    def test_web_listener_registrars_use_shared_helper(self) -> None:
        source = _web_binding_source()
        self.assertIn(
            "registerWebListener(",
            source,
            "web listener registration should be centralized",
        )
        for fn in _WEB_LISTENER_REGISTRARS:
            with self.subTest(fn=fn):
                body = _function_body(source, fn)
                self.assertIn(
                    "registerWebListener(",
                    body,
                    f"{fn} must delegate to registerWebListener",
                )
                self.assertNotIn(
                    "rememberListener(state)",
                    body,
                    f"{fn} must not duplicate listener bookkeeping",
                )

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

    def test_arm_task_tick_retries_until_scene_ready(self) -> None:
        source = _read_repo_file(_TASK_SCHEDULER)
        host_guard = "#if !defined(LUAUAPI_HOST_TESTS)"
        start = source.find(host_guard)
        self.assertNotEqual(start, -1, "missing host-test guard in TaskScheduler.cpp")
        end = source.find("#else", start)
        self.assertNotEqual(end, -1, "missing host-test else branch")
        production = source[start:end]
        self.assertIn("queueInMainThread", production)
        self.assertIn("scheduleArmRetry", production)
        self.assertIn("s_armPending = false", production)


class ManualFieldsBindingGuardTests(unittest.TestCase):
    def test_binding_emission_skips_manual_free_fn_fields(self) -> None:
        from luau_codegen.emit.bindings.free_functions import emit_free_functions_file
        from luau_codegen.parse.broma import Arg, Function

        functions: list[Function] = []
        for lua_path, entries in MANUAL_FREE_FN_FIELDS.items():
            namespace = lua_path.replace(".", "::")
            for entry in entries:
                name = entry.split(":", 1)[0].strip()
                if name[0].isupper():
                    continue
                functions.append(
                    Function(
                        name=name,
                        namespace=namespace,
                        ret="void",
                        args=[Arg("int", "a")],
                    )
                )

        out = emit_free_functions_file(functions, {}, manual_fields=MANUAL_FREE_FN_FIELDS)
        for fn in functions:
            with self.subTest(fn=f"{fn.lua_path}.{fn.name}"):
                self.assertNotIn(
                    f'lua_setfield(L, -2, "{fn.name}")',
                    out,
                    "generated C++ bindings must not register handwritten free functions",
                )

    def test_binding_emission_keeps_non_manual_free_fn_fields(self) -> None:
        from luau_codegen.emit.bindings.free_functions import emit_free_functions_file
        from luau_codegen.parse.broma import Function

        fn = Function(name="getObjectName", namespace="geode::cocos", ret="void", args=[])
        out = emit_free_functions_file([fn], {}, manual_fields=MANUAL_FREE_FN_FIELDS)
        self.assertIn('lua_setfield(L, -2, "getObjectName")', out)


class CocosHybridGuardTests(unittest.TestCase):
    def test_utils_cocos_hpp_in_free_function_manifest(self) -> None:
        headers = {rel for rel, _ns, _names in FREE_FUNCTION_SOURCES}
        self.assertIn(
            "utils/cocos.hpp",
            headers,
            "generated free-function bindings must own broad geode::cocos helpers",
        )

    def test_generated_owned_cocos_wrappers_not_in_handwritten_binding(self) -> None:
        registered = _registered_cocos_fields()
        overlap = registered & _COCOS_GENERATED_OWNERS
        self.assertFalse(
            overlap,
            "handwritten geode.cocos must keep only color/hex helpers; "
            f"remove generated-owned wrappers: {sorted(overlap)}",
        )

    def test_generated_owned_cocos_wrappers_not_in_manual_fields(self) -> None:
        declared = {
            entry.split(":", 1)[0].strip() for entry in MANUAL_FREE_FN_FIELDS.get("geode.cocos", [])
        }
        overlap = declared & _COCOS_GENERATED_OWNERS
        self.assertFalse(
            overlap,
            "manual_fields geode.cocos must not duplicate generated free-fn stubs for "
            f"{sorted(overlap)}",
        )


class CallbackFailureLoggingGuardTests(unittest.TestCase):
    def test_shared_callback_failure_logger_exists(self) -> None:
        callback_header = _read_repo_file(_LUA_CALLBACK)
        self.assertIn(
            "logCallbackFailure",
            callback_header,
            "framework bindings should share one callback failure logger",
        )

    def test_web_binding_logs_invoke_failures(self) -> None:
        source = _web_binding_source()
        self.assertIn("logWebCallbackFailure", source)
        cases = (
            ("setProgressCallback", "void"),
            ("startRequest", "std::shared_ptr<WebTask>"),
            ("invokeRequestEvent", "bool"),
            ("invokeResponseEventNow", "bool"),
        )
        for fn, ret in cases:
            with self.subTest(fn=fn):
                body = _function_body(source, fn, ret=ret)
                self.assertIn(
                    "logWebCallbackFailure",
                    body,
                    f"{fn} must log ignored LuaCallback::invoke failures",
                )

    def test_selector_handlers_log_invoke_failures(self) -> None:
        source = _read_repo_file(_LUA_SELECTOR)
        for method in (
            "LuaScheduleHandler::onSchedule",
            "LuaCallFuncHandler::onCallFunc",
            "LuaCallFuncNHandler::onCallFuncN",
            "LuaCallFuncNDHandler::onCallFuncND",
            "LuaCallFuncOHandler::onCallFuncO",
        ):
            with self.subTest(method=method):
                class_name, _, fn_name = method.partition("::")
                marker = f"void {class_name}::{fn_name}("
                start = source.find(marker)
                self.assertNotEqual(start, -1, f"missing {method}")
                end = source.find("\n    void ", start + len(marker))
                if end == -1:
                    end = len(source)
                body = source[start:end]
                self.assertRegex(
                    body,
                    r"if\s*\(\s*!.*invoke\(",
                    f"{method} must check invoke() and log failures",
                )
                self.assertIn(
                    "logCallbackFailure",
                    body,
                    f"{method} must call shared logCallbackFailure",
                )

    def test_menu_handler_logs_invoke_failures(self) -> None:
        source = _read_repo_file(_LUA_MENU)
        body = _function_body(source, "LuaMenuHandler::onCallback", ret="void")
        self.assertRegex(body, r"if\s*\(\s*!.*invoke\(")
        self.assertIn("logCallbackFailure", body)

    def test_delegate_table_invoke_logs_failures(self) -> None:
        source = _read_repo_file(_LUA_DELEGATE)
        body = _function_body(source, "invokeTableField", ret="bool")
        self.assertIn("logCallbackFailure", body)

    def test_mod_setting_listeners_log_invoke_failures(self) -> None:
        source = _read_repo_file(_GEODE_MOD)
        for fn in ("modListenForSettingChanges", "modListenForAllSettingChanges"):
            with self.subTest(fn=fn):
                body = _function_body(source, fn)
                self.assertRegex(body, r"if\s*\(\s*!.*invoke\(")
                self.assertIn("logCallbackFailure", body)

    def test_permission_request_logs_invoke_failures(self) -> None:
        source = _read_repo_file(_GEODE_PERMISSION)
        body = _function_body(source, "permRequest")
        self.assertIn("logCallbackFailure", body)


class HandleGcGuardTests(unittest.TestCase):
    def test_task_handle_metatable_registers_gc_cancellation(self) -> None:
        source = _read_repo_file(_TASK_BINDING)
        self.assertIn('"__gc"', source)
        gc_match = re.search(
            r"int\s+(\w+Gc)\(lua_State\* L\)\s*\{[^}]*TaskScheduler::get\(\)\.cancel",
            source,
            re.DOTALL,
        )
        self.assertIsNotNone(
            gc_match,
            "task handle __gc must cancel the scheduled task id",
        )
        register_body = _function_body(source, "registerHandleMetatable", ret="void")
        self.assertIn('"__gc"', register_body)

    def test_imgui_handle_metatable_registers_gc_cancellation(self) -> None:
        source = _read_repo_file(_IMGUI_BINDING)
        self.assertIn('"__gc"', source)
        gc_match = re.search(
            r"int\s+(\w+Gc)\(lua_State\* L\)\s*\{[^}]*ImGuiDrawScheduler::get\(\)\.cancel",
            source,
            re.DOTALL,
        )
        self.assertIsNotNone(
            gc_match,
            "imgui handle __gc must cancel the draw callback id",
        )
        register_body = _function_body(source, "registerHandleMetatable", ret="void")
        self.assertIn('"__gc"', register_body)


class FreeFnManifestSyncTests(unittest.TestCase):
    def test_scanner_and_emitter_share_one_manifest(self) -> None:
        from luau_codegen.model.free_fn_sources import free_function_includes
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
