from __future__ import annotations

import unittest

from binding_guard_support import (
    CANCELLABLE_SLOTS,
    COCOS_GENERATED_OWNERS,
    FS_BINDING,
    GEODE_MOD,
    GEODE_PERMISSION,
    IMGUI_BINDING,
    JSON_CONVERT,
    LUA_CALLBACK,
    LUA_COCOS,
    LUA_DELEGATE,
    SCHEDULED_HANDLE_BINDING,
    TASK_BINDING,
    TASK_SCHEDULER,
    USERTYPE,
    function_body,
    read_repo_file,
    registered_cocos_fields,
    web_binding_source,
)
from luau_codegen.emit.luau_types.manual_fields import (  # type: ignore[import-unresolved]
    MANUAL_FREE_FN_FIELDS,
)
from luau_codegen.model.free_fn_sources import FREE_FUNCTION_SOURCES  # type: ignore[import-unresolved]


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
        registered = registered_cocos_fields()
        overlap = registered & COCOS_GENERATED_OWNERS
        self.assertFalse(
            overlap,
            "handwritten geode.cocos must keep only color/hex helpers; "
            f"remove generated-owned wrappers: {sorted(overlap)}",
        )

    def test_generated_owned_cocos_wrappers_not_in_manual_fields(self) -> None:
        declared = {
            entry.split(":", 1)[0].strip() for entry in MANUAL_FREE_FN_FIELDS.get("geode.cocos", [])
        }
        overlap = declared & COCOS_GENERATED_OWNERS
        self.assertFalse(
            overlap,
            "manual_fields geode.cocos must not duplicate generated free-fn stubs for "
            f"{sorted(overlap)}",
        )


class CallbackFailureLoggingGuardTests(unittest.TestCase):
    def test_shared_callback_failure_logger_exists(self) -> None:
        callback_header = read_repo_file(LUA_CALLBACK)
        self.assertIn(
            "logCallbackFailure",
            callback_header,
            "framework bindings should share one callback failure logger",
        )

    def test_web_binding_logs_invoke_failures(self) -> None:
        source = web_binding_source()
        self.assertIn("logCallbackFailure", source)
        cases = (
            ("setProgressCallback", "void"),
            ("startRequest", "std::shared_ptr<WebTask>"),
            ("invokeRequestEvent", "bool"),
            ("invokeResponseEventNow", "bool"),
        )
        for fn, ret in cases:
            with self.subTest(fn=fn):
                body = function_body(source, fn, ret=ret)
                self.assertIn(
                    "logCallbackFailure",
                    body,
                    f"{fn} must log ignored LuaCallback::invoke failures",
                )

    def test_selector_handlers_log_invoke_failures(self) -> None:
        source = read_repo_file(LUA_COCOS)
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
                self.assertIn("invokeCocosCallback", body)

    def test_menu_handler_logs_invoke_failures(self) -> None:
        source = read_repo_file(LUA_COCOS)
        body = function_body(source, "LuaMenuHandler::onCallback", ret="void")
        self.assertIn("invokeCocosCallback", body)

    def test_delegate_table_invoke_logs_failures(self) -> None:
        source = read_repo_file(LUA_DELEGATE)
        body = function_body(source, "invokeTableField", ret="bool")
        self.assertIn("logCallbackFailure", body)

    def test_mod_setting_listeners_log_invoke_failures(self) -> None:
        source = read_repo_file(GEODE_MOD)
        for fn in ("modListenForSettingChanges", "modListenForAllSettingChanges"):
            with self.subTest(fn=fn):
                body = function_body(source, fn)
                self.assertRegex(body, r"if\s*\(\s*!.*invoke\(")
                self.assertIn("logCallbackFailure", body)

    def test_permission_request_logs_invoke_failures(self) -> None:
        source = read_repo_file(GEODE_PERMISSION)
        body = function_body(source, "permRequest")
        self.assertIn("logCallbackFailure", body)


class HandleGcGuardTests(unittest.TestCase):
    def test_scheduled_handle_binding_cancels_via_scheduler(self) -> None:
        source = read_repo_file(SCHEDULED_HANDLE_BINDING)
        self.assertIn("Scheduler::get().cancel", source)
        self.assertIn("lua_newuserdatataggedwithmetatable", source)
        self.assertIn("registerTaggedMetatable", source)

    def test_task_handle_metatable_registers_gc_cancellation(self) -> None:
        source = read_repo_file(TASK_BINDING)
        self.assertIn("ScheduledHandleBinding", source)
        self.assertIn("detail::taskHandleTag()", source)
        self.assertIn("TaskHandleBinding::registerMetatable", source)

    def test_imgui_handle_metatable_registers_gc_cancellation(self) -> None:
        source = read_repo_file(IMGUI_BINDING)
        self.assertIn("ScheduledHandleBinding", source)
        self.assertIn("detail::imguiDrawHandleTag()", source)
        self.assertIn("ImGuiDrawHandleBinding::registerMetatable", source)

    def test_schedulers_fire_callbacks_via_framework_helper(self) -> None:
        callback_source = read_repo_file(LUA_CALLBACK)
        self.assertIn("static bool fire(LuaRef& callback", callback_source)
        self.assertIn("protectedCall", callback_source)

        task_scheduler = read_repo_file(TASK_SCHEDULER)
        self.assertIn("LuaCallback::fire", task_scheduler)

        imgui_scheduler = read_repo_file(IMGUI_BINDING)
        self.assertIn("LuaCallback::fire", imgui_scheduler)

    def test_schedulers_store_slots_via_indexed_slot_map(self) -> None:
        slots_source = read_repo_file(CANCELLABLE_SLOTS)
        self.assertIn("compactCancelledSlots", slots_source)
        self.assertIn("IndexedSlotMap", slots_source)

        task_scheduler = read_repo_file(TASK_SCHEDULER)
        self.assertIn("IndexedSlotMap", task_scheduler)

        imgui_scheduler = read_repo_file("src/bindings/imgui/ImGuiDrawScheduler.hpp")
        self.assertIn("IndexedSlotMap", imgui_scheduler)
        self.assertIn("compactCancelledSlots", read_repo_file(IMGUI_BINDING))


class ErrorSemanticsGuardTests(unittest.TestCase):
    def test_usertype_field_get_raises_on_failure(self) -> None:
        source = read_repo_file(USERTYPE)
        index_body = function_body(source, "usertypeIndex")
        self.assertIn(
            "tryInvokeFieldAccessor",
            index_body,
            "usertype field get must delegate to the shared field accessor",
        )
        self.assertIn(
            '"usertype field get failed"',
            index_body,
            "usertype field get must supply a failure message to the accessor",
        )
        helper_start = source.find("bool tryInvokeFieldAccessor")
        self.assertNotEqual(helper_start, -1, "missing tryInvokeFieldAccessor helper")
        helper_end = source.find("int usertypeIndex", helper_start)
        helper_body = source[helper_start:helper_end]
        self.assertIn(
            'luaL_error(L, "%s", failMsg)',
            helper_body,
            "field accessor must raise on protected-call failure, not silently return nil",
        )
        self.assertNotRegex(
            helper_body,
            r"invokeFieldAccessor\([^)]*\)\)\s*\{\s*return true;\s*\}\s*lua_settop\(L, top\);\s*lua_pushnil",
            "field accessor must not silently push nil on protected-call failure",
        )

    def test_fs_exists_returns_error_on_filesystem_failure(self) -> None:
        source = read_repo_file(FS_BINDING)
        body = function_body(source, "fsExists")
        self.assertIn("if (ec)", body)
        self.assertIn("return 2", body)

    def test_json_convert_reports_depth_overflow(self) -> None:
        source = read_repo_file(JSON_CONVERT)
        self.assertIn("kJsonDepthExceededMsg", source)
        self.assertIn("geode::Result<void> pushJson", source)
        self.assertIn("geode::Result<matjson::Value> toJson", source)

    def test_mod_set_saved_value_checks_conversion_and_container(self) -> None:
        source = read_repo_file(GEODE_MOD)
        body = function_body(source, "modSetSavedValue")
        self.assertIn("valueResult.isErr()", body)
        self.assertIn("isObject()", body)
        self.assertGreaterEqual(body.count("pushNilErr"), 2)

    def test_delegate_default_return_policy_documented(self) -> None:
        source = read_repo_file("docs/reference/lua/delegates.md")
        self.assertIn("logs the failure", source)
        self.assertIn("method default", source)


class FreeFnManifestSyncTests(unittest.TestCase):
    def test_free_function_manifest_matches_emitter(self) -> None:
        from luau_codegen.emit.bindings.free_functions import emit_free_functions_file
        from luau_codegen.model.free_fn_sources import free_function_includes

        includes = free_function_includes()
        self.assertEqual(
            len(includes),
            len(FREE_FUNCTION_SOURCES),
            "one include per scanned free-function header",
        )

        emitted = emit_free_functions_file([], {})
        for (rel, _ns, _names), inc in zip(FREE_FUNCTION_SOURCES, includes):
            self.assertEqual(inc, f"Geode/{rel}")
            self.assertIn(
                f"#include <{inc}>",
                emitted,
                f"generated free-function file must #include {inc}",
            )
