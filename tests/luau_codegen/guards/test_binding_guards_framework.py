from __future__ import annotations

import os
import unittest

from binding_guard_support import (
    CANCELLABLE_SLOTS,
    COCOS_GENERATED_OWNERS,
    FS_BINDING,
    GEODE_MOD,
    GEODE_PERMISSION,
    GEODE_TASK_HANDLE_BINDING,
    IMGUI_BINDING,
    JSON_CONVERT,
    LUA_CALLBACK,
    LUA_COCOS,
    LUA_DELEGATE,
    SCHEDULED_HANDLE_BINDING,
    TASK_BINDING,
    TASK_SCHEDULER,
    USERTYPE,
    _REPO_ROOT,
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

    def test_invoke_cocos_callback_retain_handler_in_invoke_scope(self) -> None:
        source = read_repo_file(LUA_COCOS)
        body = function_body(source, "invokeCocosCallback", ret="bool")
        self.assertRegex(body, r"ScopedRetain\s+keepHandler\s*\(\s*handler\s*\)")
        self.assertIn("->retain()", source)
        self.assertNotIn("DeferredSingleRelease", body)
        self.assertRegex(body, r"auto\s+cb\s*=\s*callback")

    def test_anchor_trampoline_binds_handler_anchor(self) -> None:
        source = read_repo_file("src/framework/callback/LuaTrampolineRegistry.cpp")
        body = function_body(source, "anchorTrampoline", ret="void")
        self.assertIn("bindAnchor", body)

    def test_invoke_cocos_callback_skips_dead_anchor(self) -> None:
        source = read_repo_file(LUA_COCOS)
        body = function_body(source, "invokeCocosCallback", ret="bool")
        self.assertIn("anchorAlive", body)

    def test_menu_handler_passes_this_to_invoke(self) -> None:
        source = read_repo_file(LUA_COCOS)
        body = function_body(source, "LuaMenuHandler::onCallback", ret="void")
        self.assertRegex(body, r"if\s*\(\s*!anchorAlive\(\)\s*\)\s*return")
        self.assertRegex(body, r"invokeCocosCallback\(\s*this,\s*m_callback,")
        self.assertNotRegex(body, r"ScopedRetain\s+keepThis")
        self.assertNotRegex(body, r"ScopedRetain\s+keepSender")
        self.assertNotIn("pushBorrowed", body)
        self.assertIn("pushCallbackArg", body)
        self.assertNotIn("DeferredSingleRelease", body)
        self.assertNotIn("auto callback = m_callback", body)
        self.assertRegex(body, r"invokeCocosCallback\([\s\S]*,\s*sender\s*\)")

    def test_handler_entrypoints_check_anchor_no_local_retain(self) -> None:
        source = read_repo_file(LUA_COCOS)
        for method in (
            "LuaMenuHandler::onCallback",
            "LuaScheduleHandler::onSchedule",
            "LuaCallFuncHandler::onCallFunc",
            "LuaCallFuncNHandler::onCallFuncN",
            "LuaCallFuncNDHandler::onCallFuncND",
            "LuaCallFuncOHandler::onCallFuncO",
        ):
            with self.subTest(method=method):
                body = function_body(source, method, ret="void")
                self.assertIn("anchorAlive()", body, f"{method} must check anchorAlive")
                self.assertNotRegex(
                    body,
                    r"ScopedRetain\s+keepThis",
                    f"{method} must not retain handler locally",
                )

    def test_invoke_cocos_callback_retain_arg_in_invoke_scope(self) -> None:
        source = read_repo_file(LUA_COCOS)
        body = function_body(source, "invokeCocosCallback", ret="bool")
        self.assertIn("argKeepalive", body)
        self.assertRegex(body, r"ScopedRetain\s+keepHandler\s*\(\s*handler\s*\)")
        self.assertRegex(body, r"ScopedRetain\s+keepArg\s*\(\s*argKeepalive\s*\)")
        self.assertNotRegex(body, r"ScopedRetain\s+keepSender")
        self.assertNotRegex(body, r"ScopedRetain\s+keepNode")
        self.assertNotRegex(body, r"ScopedRetain\s+keepObj")

    def test_scoped_retain_dtor_no_weakref(self) -> None:
        source = read_repo_file(LUA_COCOS)
        dtor_start = source.find("~ScopedRetain()")
        self.assertNotEqual(dtor_start, -1, "missing ScopedRetain dtor")
        dtor_end = source.find("\n        };", dtor_start)
        dtor_body = source[dtor_start:dtor_end]
        self.assertNotIn("WeakRef", dtor_body)

    def test_menu_callback_arg_uses_ephemeral_push(self) -> None:
        usertype_source = read_repo_file(USERTYPE)
        dtor_body = function_body(usertype_source, "destructorDispatch", ret="void")
        self.assertIn("kUserdataEphemeralFlag", dtor_body)
        self.assertNotIn("evictTrampolinesForAnchor", dtor_body)
        self.assertIn("pushCallbackArg", usertype_source)
        self.assertIn("kUserdataEphemeralFlag", usertype_source)

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

    def test_geode_task_handles_poll_from_task_tick(self) -> None:
        source = read_repo_file(TASK_SCHEDULER)
        self.assertIn('#include "bindings/geode/GeodeTaskHandleBinding.hpp"', source)
        self.assertIn("pollGeodeTaskHandles(L)", source)
        self.assertLess(
            source.find("TaskScheduler::get().advance"),
            source.find("pollGeodeTaskHandles(L)"),
        )

    def test_geode_task_handle_bridge_in_host_test_sources(self) -> None:
        test_sources = read_repo_file("cmake/TestSources.cmake")
        self.assertIn(GEODE_TASK_HANDLE_BINDING, test_sources)

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


class CastConventionGuardTests(unittest.TestCase):
    def test_fields_evict_uses_typeinfo_cast(self) -> None:
        source = read_repo_file("src/framework/usertype/Fields.cpp")
        start = source.find("void Fields::evict(cocos2d::CCObject* object)")
        self.assertNotEqual(start, -1, "missing void Fields::evict(cocos2d::CCObject* object)")
        rest = source[start + len("void Fields::evict(cocos2d::CCObject* object)") :]
        next_fn = rest.find("\n    void Fields::")
        body = source[
            start : start
            + len("void Fields::evict(cocos2d::CCObject* object)")
            + (next_fn if next_fn != -1 else len(rest))
        ]
        self.assertIn("geode::cast::typeinfo_cast<cocos2d::CCNode*>", body)
        self.assertNotIn("reinterpret_cast", body)

    def test_fields_evict_if_final_release_skips_typeinfo_cast(self) -> None:
        source = read_repo_file("src/framework/usertype/Fields.cpp")
        start = source.find("void Fields::evictIfFinalRelease(cocos2d::CCObject* object)")
        self.assertNotEqual(
            start,
            -1,
            "missing void Fields::evictIfFinalRelease(cocos2d::CCObject* object)",
        )
        rest = source[start + len("void Fields::evictIfFinalRelease(cocos2d::CCObject* object)") :]
        next_fn = rest.find("\n    void Fields::")
        body = source[
            start : start
            + len("void Fields::evictIfFinalRelease(cocos2d::CCObject* object)")
            + (next_fn if next_fn != -1 else len(rest))
        ]
        self.assertNotIn("geode::cast::typeinfo_cast", body)
        self.assertIn("fieldTables()", body)
        self.assertIn("entryStillOwnsNode", body)

    def test_drop_borrowed_target_if_final_release_checks_set_before_retain_count(
        self,
    ) -> None:
        source = read_repo_file("src/framework/usertype/Usertype.cpp")
        start = source.find("void dropBorrowedTargetIfFinalRelease(cocos2d::CCObject* object)")
        self.assertNotEqual(
            start,
            -1,
            "missing void dropBorrowedTargetIfFinalRelease(cocos2d::CCObject* object)",
        )
        rest = source[
            start + len("void dropBorrowedTargetIfFinalRelease(cocos2d::CCObject* object)") :
        ]
        next_fn = rest.find("\n} // namespace luax")
        body = source[
            start : start
            + len("void dropBorrowedTargetIfFinalRelease(cocos2d::CCObject* object)")
            + (next_fn if next_fn != -1 else len(rest))
        ]
        self.assertIn("borrowedTargets()", body)
        self.assertIn("retainCount()", body)
        self.assertLess(body.find("borrowedTargets()"), body.find("retainCount()"))

    def test_trampoline_anchor_uses_static_cast_not_typeinfo(self) -> None:
        source = read_repo_file("src/framework/callback/LuaTrampolineRegistry.cpp")
        body = function_body(source, "anchorTrampoline", ret="void")
        self.assertIn("static_cast<LuaCocosHandlerBase*>", body)
        self.assertNotIn("typeinfo_cast", body)

    def test_no_reinterpret_cast_cocos2d_in_framework_or_render3d(self) -> None:
        roots = ("src/framework", "src/render3d")
        offenders: list[str] = []
        for root in roots:
            for dirpath, _dirnames, filenames in os.walk(
                os.path.join(_REPO_ROOT, root.replace("/", os.sep))
            ):
                for filename in filenames:
                    if not filename.endswith((".cpp", ".hpp", ".h")):
                        continue
                    rel = os.path.relpath(os.path.join(dirpath, filename), _REPO_ROOT)
                    text = read_repo_file(rel.replace("\\", "/"))
                    if "reinterpret_cast<cocos2d::" in text:
                        offenders.append(rel.replace("\\", "/"))
        self.assertFalse(
            offenders,
            "framework/render3d must not reinterpret_cast cocos2d types; "
            f"use geode::cast::typeinfo_cast: {offenders}",
        )


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
