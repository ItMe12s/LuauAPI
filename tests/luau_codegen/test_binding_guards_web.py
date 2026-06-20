from __future__ import annotations

import unittest

from binding_guard_support import (
    CONFIG_HEADER,
    FS_BINDING,
    MAIN_CPP,
    MULTIPART_CUMULATIVE_BODY_METHODS,
    REQUEST_BODY_APPLY_HELPERS,
    REQUEST_BODY_CAP_CHECKS,
    REQUEST_BODY_METHODS,
    TASK_BINDING,
    TASK_SCHEDULER,
    WEB_INTERNAL,
    WEB_LISTENER_REGISTRARS,
    function_body,
    inline_function_body,
    read_repo_file,
    task_scheduler_production_branch,
    web_binding_source,
)


class BindingGuardTests(unittest.TestCase):
    def test_web_response_methods_enforce_size_cap(self) -> None:
        config = read_repo_file(CONFIG_HEADER)
        self.assertIn("kMaxWebResponseBytes", config)

        source = web_binding_source()
        for method in (
            "responseText",
            "responseBytes",
            "responseSaveTo",
            "responseJson",
        ):
            with self.subTest(method=method):
                body = function_body(source, method)
                self.assertIn(
                    "responseDataWithinLimit",
                    body,
                    f"{method} must enforce kMaxWebResponseBytes",
                )

    def test_push_response_enforces_size_cap_before_boxing(self) -> None:
        source = read_repo_file(WEB_INTERNAL)
        body = inline_function_body(source, "inline void pushResponse(lua_State* L")
        self.assertIn(
            "kMaxWebResponseBytes",
            body,
            "pushResponse must reject oversized bodies before boxing userdata",
        )

    def test_start_request_enforces_concurrent_request_cap(self) -> None:
        config = read_repo_file(CONFIG_HEADER)
        self.assertIn("kMaxWebConcurrentRequests", config)
        self.assertIn("kMaxWebRequestBytes", config)

        source = web_binding_source()
        body = function_body(source, "startRequest", ret="std::shared_ptr<WebTask>")
        self.assertIn(
            "kMaxWebConcurrentRequests",
            body,
            "startRequest must enforce kMaxWebConcurrentRequests",
        )
        self.assertIn(
            "countInflightWebRequests",
            body,
            "startRequest must count in-flight requests instead of stale handles",
        )
        send_pos = body.find("req.send(")
        cap_pos = body.find("kMaxWebConcurrentRequests")
        self.assertNotEqual(send_pos, -1, "startRequest must call req.send")
        self.assertNotEqual(cap_pos, -1, "startRequest must reference kMaxWebConcurrentRequests")
        self.assertLess(
            cap_pos,
            send_pos,
            "startRequest must reject over-cap requests before req.send",
        )

    def test_apply_options_enforces_request_body_cap(self) -> None:
        source = web_binding_source()
        body = function_body(source, "applyOptions", ret="void")
        for helper in REQUEST_BODY_APPLY_HELPERS:
            with self.subTest(helper=helper):
                self.assertIn(
                    helper,
                    body,
                    "applyOptions must delegate request bodies to shared apply* helpers",
                )
                helper_body = function_body(source, helper, ret="bool")
                self.assertTrue(
                    any(check in helper_body for check in REQUEST_BODY_CAP_CHECKS),
                    f"{helper} must enforce kMaxWebRequestBytes",
                )

    def test_request_body_methods_enforce_size_cap(self) -> None:
        source = web_binding_source()
        for method in REQUEST_BODY_METHODS:
            with self.subTest(method=method):
                body = function_body(source, method)
                if method in REQUEST_BODY_APPLY_HELPERS or method.startswith("requestBody"):
                    helper = {
                        "requestBody": "applyBody",
                        "requestBodyString": "applyBodyString",
                        "requestBodyJson": "applyBodyJson",
                        "requestBodyMultipart": "applyBodyMultipart",
                    }.get(method)
                    if helper is not None:
                        self.assertIn(
                            helper,
                            body,
                            f"{method} must delegate body size checks to {helper}",
                        )
                        helper_body = function_body(source, helper, ret="bool")
                        self.assertTrue(
                            any(check in helper_body for check in REQUEST_BODY_CAP_CHECKS),
                            f"{helper} must enforce kMaxWebRequestBytes",
                        )
                        continue
                self.assertTrue(
                    any(check in body for check in REQUEST_BODY_CAP_CHECKS),
                    f"{method} must enforce kMaxWebRequestBytes",
                )

    def test_multipart_file_methods_enforce_cumulative_body_cap(self) -> None:
        source = web_binding_source()
        param_body = function_body(source, "multipartParam")
        self.assertIn(
            "getBody().size()",
            param_body,
            "multipartParam is the canonical cumulative cap pattern",
        )
        for method in MULTIPART_CUMULATIVE_BODY_METHODS:
            with self.subTest(method=method):
                body = function_body(source, method)
                append = body.find("form.file(")
                self.assertNotEqual(
                    append,
                    -1,
                    f"{method} must append via form.file",
                )
                after_append = body[append:]
                self.assertIn(
                    "getBody().size()",
                    after_append,
                    f"{method} must check cumulative body size after form.file append",
                )
                self.assertIn(
                    "requestBodyWithinLimit",
                    after_append,
                    f"{method} must enforce kMaxWebRequestBytes on cumulative body",
                )

    def test_async_request_callback_rejects_oversized_response(self) -> None:
        source = web_binding_source()
        body = function_body(source, "startRequest", ret="std::shared_ptr<WebTask>")
        self.assertIn(
            "responseDataWithinLimit",
            body,
            "async request callbacks must reject oversized responses before pushResponse",
        )

    def test_response_listener_rejects_oversized_response(self) -> None:
        source = web_binding_source()
        body = function_body(source, "invokeResponseEventNow", ret="bool")
        self.assertIn(
            "responseDataWithinLimit",
            body,
            "response listeners must reject oversized responses before pushResponse",
        )

    def test_off_thread_request_intercept_fails_closed(self) -> None:
        source = read_repo_file("src/bindings/geode/web/GeodeWebListeners.cpp")
        body = function_body(source, "invokeRequestEvent", ret="bool")
        self.assertIn(
            "!Runtime::isMainThread()",
            body,
            "invokeRequestEvent must detect off-thread intercept events",
        )
        self.assertIn(
            "off-thread intercept blocked",
            body,
            "off-thread intercept must log a one-time block warning",
        )
        self.assertIn(
            "loggedOffThreadBlock",
            body,
            "off-thread intercept warning must only fire once",
        )
        off_thread = body.split("!Runtime::isMainThread()")[1].split("struct Ctx")[0]
        self.assertIn(
            "return true",
            off_thread,
            "off-thread intercept must fail closed and block the request",
        )

    def test_request_intercept_callback_failure_fails_closed(self) -> None:
        source = read_repo_file("src/bindings/geode/web/GeodeWebListeners.cpp")
        body = function_body(source, "invokeRequestEvent", ret="bool")
        failure = body.split("if (!ok)")[1]
        self.assertIn(
            "return true",
            failure,
            "intercept callback failures must fail closed and block the request",
        )

    def test_off_thread_response_listener_is_side_effects_only(self) -> None:
        source = read_repo_file("src/bindings/geode/web/GeodeWebListeners.cpp")
        body = function_body(source, "invokeResponseEvent", ret="bool")
        self.assertIn(
            "queueInMainThread",
            body,
            "off-thread response listeners must defer to the main thread",
        )
        self.assertIn(
            "side effects",
            body.lower(),
            "invokeResponseEvent must document side-effects-only off-thread semantics",
        )
        self.assertIn(
            "(void)invokeResponseEventNow",
            body,
            "off-thread response listeners must discard the synchronous stop result",
        )
        self.assertIn(
            "return false",
            body,
            "off-thread response listeners must not stop propagation synchronously",
        )

    def test_geode_web_binding_does_not_use_luaL_register(self) -> None:
        source = web_binding_source()
        self.assertNotIn(
            "luaL_register",
            source,
            "GeodeWebCore must register metatable methods without luaL_register",
        )

    def test_web_internal_has_no_dead_checkudata_null_checks(self) -> None:
        source = read_repo_file(WEB_INTERNAL)
        self.assertNotIn(
            "if (!box) luaL_error",
            source,
            "luaL_checkudata never returns null; redundant checks are dead code",
        )

    def test_web_listener_registrars_use_shared_helper(self) -> None:
        source = web_binding_source()
        self.assertIn(
            "kListenerDescriptors",
            source,
            "web listener registration should use a descriptor table",
        )
        body = function_body(source, "registerListenerFromDescriptor")
        self.assertIn(
            "registerWebListener(",
            body,
            "registerListenerFromDescriptor must delegate to registerWebListener",
        )
        self.assertNotIn(
            "rememberListener(state)",
            body,
            "registerListenerFromDescriptor must not duplicate listener bookkeeping",
        )
        for fn in WEB_LISTENER_REGISTRARS:
            with self.subTest(fn=fn):
                wrapper = function_body(source, fn)
                self.assertIn(
                    "listenerDispatch(",
                    wrapper,
                    f"{fn} must delegate to listenerDispatch",
                )
                self.assertNotIn(
                    "rememberListener(state)",
                    wrapper,
                    f"{fn} must not duplicate listener bookkeeping",
                )

    def test_fs_write_enforces_size_cap(self) -> None:
        config = read_repo_file(CONFIG_HEADER)
        self.assertIn("kMaxFsWriteBytes", config)

        source = read_repo_file(FS_BINDING)
        body = function_body(source, "fsWrite")
        self.assertIn(
            "kMaxFsWriteBytes",
            body,
            "fsWrite must enforce kMaxFsWriteBytes",
        )

    def test_arm_task_tick_logs_director_and_scheduler_failures(self) -> None:
        production = task_scheduler_production_branch()
        self.assertIn("CCDirector unavailable", production)
        self.assertIn("cocos2d scheduler unavailable", production)

    def test_arm_task_tick_returns_bool(self) -> None:
        source = read_repo_file(TASK_SCHEDULER)
        self.assertIn("bool armTaskTick()", source)

    def test_host_tests_stub_arm_task_tick(self) -> None:
        source = read_repo_file(TASK_SCHEDULER)
        else_start = source.find("#else")
        host_branch = source[else_start:]
        self.assertIn("bool armTaskTick()", host_branch)
        self.assertIn("return false;", host_branch)
        self.assertIn("void disarmTaskTick() {}", host_branch)

    def test_task_schedulers_rearm_on_each_call(self) -> None:
        source = read_repo_file(TASK_BINDING)
        for fn in ("taskDelay", "taskEvery", "taskDefer"):
            with self.subTest(fn=fn):
                body = function_body(source, fn)
                self.assertIn(
                    "ensureTaskTickArmed",
                    body,
                    f"{fn} must lazily re-arm the scheduler tick",
                )

    def test_arm_task_tick_retries_until_scene_ready(self) -> None:
        production = task_scheduler_production_branch()
        self.assertIn("queueInMainThread", production)
        self.assertIn("scheduleArmRetry", production)
        self.assertIn("s_armPending = false", production)

    def test_mod_loaded_runs_startup_inline(self) -> None:
        source = read_repo_file(MAIN_CPP)
        loaded_start = source.find("$on_mod(Loaded)")
        self.assertNotEqual(loaded_start, -1, "missing $on_mod(Loaded) handler")
        loaded_end = source.find("$on_game(", loaded_start)
        self.assertNotEqual(loaded_end, -1, "missing $on_game handler after Loaded")
        loaded_body = source[loaded_start:loaded_end]
        self.assertNotIn(
            "queueInMainThread",
            loaded_body,
            "Loaded already runs on the main thread, queue would defer startup",
        )
        set_pos = loaded_body.find("setMainThreadId")
        create_pos = loaded_body.find("getOrCreate")
        run_pos = loaded_body.find("runFile")
        for symbol, pos in (
            ("setMainThreadId", set_pos),
            ("getOrCreate", create_pos),
            ("runFile", run_pos),
        ):
            self.assertNotEqual(pos, -1, f"missing {symbol} in Loaded handler")
        self.assertLess(set_pos, create_pos, "setMainThreadId must run before getOrCreate")
        self.assertLess(create_pos, run_pos, "getOrCreate must run before runFile")
