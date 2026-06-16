from __future__ import annotations

import os
import re
import unittest

from luau_codegen.emit.luau_types.manual_fields import (  # type: ignore[import-unresolved]
    MANUAL_FREE_FN_FIELDS,
)
from luau_codegen.model.free_fn_sources import FREE_FUNCTION_SOURCES  # type: ignore[import-unresolved]

_REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

_WEB_BINDING = "src/bindings/geode/web/GeodeWebBinding.cpp"
_WEB_BINDING_SOURCES = (
    "src/bindings/geode/web/GeodeWebBinding.cpp",
    "src/bindings/geode/web/GeodeWebApply.cpp",
    "src/bindings/geode/web/GeodeWebOptions.cpp",
    "src/bindings/geode/web/GeodeWebRequest.cpp",
    "src/bindings/geode/web/GeodeWebResponse.cpp",
    "src/bindings/geode/web/GeodeWebMultipart.cpp",
    "src/bindings/geode/web/GeodeWebListeners.cpp",
)

_REQUEST_BODY_APPLY_HELPERS = (
    "applyBody",
    "applyBodyString",
    "applyBodyJson",
    "applyBodyMultipart",
)
_WEB_INTERNAL = "src/bindings/geode/web/WebInternal.hpp"
_FS_BINDING = "src/bindings/geode/GeodeFsBinding.cpp"
_WEBSOCKET_CONNECTION = "src/bindings/websocket/WebSocketConnection.cpp"
_WEBSOCKET_SERVER = "src/bindings/websocket/WebSocketServer.cpp"
_WEBSOCKET_INTERNAL = "src/bindings/websocket/WebSocketInternal.hpp"
_RENDERER3D_LIFETIME = "src/render3d/gpu/Renderer3DResourceLifetime.cpp"
_RENDERER3D = "src/render3d/gpu/Renderer3D.cpp"
_RENDERER3D_MESH_CACHE = "src/render3d/gpu/Renderer3DMeshCache.cpp"
_RENDERER3D_GL_UTIL = "src/render3d/gpu/GlUtil.cpp"
_RENDERER3D_TEXTURE2D = "src/render3d/gpu/Texture2D.cpp"
_CC_VIEWPORT_FRAME = "src/render3d/viewport/CCViewportFrame.cpp"
_COCOS_BINDING = "src/bindings/geode/GeodeCocosBinding.cpp"
_TASK_SCHEDULER = "src/bindings/task/TaskScheduler.cpp"
_TASK_BINDING = "src/bindings/task/TaskBinding.cpp"
_IMGUI_BINDING = "src/bindings/imgui/ImGuiDrawHandleBinding.cpp"
_IMGUI_BINDING_CPP = "src/bindings/imgui/ImGuiBinding.cpp"
_IMGUI_HOST_STUB = "tests/host/ImGuiHostStub.cpp"
_IMGUI_STYLE = "src/bindings/imgui/ImGuiStyle.cpp"
_IMGUI_BINDING_TESTS = "tests/imgui_binding_tests.cpp"
_IMGUI_HEADLESS_CMAKE = "cmake/ImGuiHeadless.cmake"
_IMGUI_TEST_HARNESS = "tests/host/ImGuiTestHarness.hpp"
_CMAKE_LISTS = "CMakeLists.txt"
_SCHEDULED_HANDLE_BINDING = "src/framework/schedule/ScheduledHandleBinding.hpp"
_SCHEDULED_CALLBACK = "src/framework/schedule/ScheduledCallback.hpp"
_SCHEDULED_SLOT_STORE = "src/framework/schedule/ScheduledSlotStore.hpp"
_LUA_SELECTOR = "src/framework/callback/LuaSelectorHandler.cpp"
_LUA_MENU = "src/framework/callback/LuaMenuHandler.cpp"
_LUA_DELEGATE = "src/framework/callback/LuaDelegate.cpp"
_LUA_CALLBACK = "src/framework/callback/LuaCallback.hpp"
_USERTYPE = "src/framework/usertype/Usertype.cpp"
_JSON_CONVERT = "src/bindings/geode/JsonConvert.cpp"
_GEODE_MOD = "src/bindings/geode/GeodeModBinding.cpp"
_GEODE_PERMISSION = "src/bindings/geode/GeodeSmallBindings.cpp"
_CONFIG_HEADER = "src/core/Config.hpp"

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

_MULTIPART_CUMULATIVE_BODY_METHODS = (
    "multipartFile",
    "multipartFileFrom",
)


def _web_binding_source() -> str:
    return "\n".join(_read_repo_file(path) for path in _WEB_BINDING_SOURCES)


def _read_repo_file(rel_path: str) -> str:
    path = os.path.join(_REPO_ROOT, rel_path)
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def _function_body(source: str, name: str, *, ret: str = "int") -> str:
    if "::" in name:
        pattern = rf"{re.escape(ret)}\s+{re.escape(name)}\s*\("
    else:
        pattern = rf"{re.escape(ret)}\s+(?:[\w:]+::)?{re.escape(name)}\s*\("
    match = re.search(pattern, source)
    assert match is not None, f"missing {ret} {name} implementation"
    start = match.start()
    marker = match.group(0)
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

    def test_start_request_enforces_concurrent_request_cap(self) -> None:
        config = _read_repo_file(_CONFIG_HEADER)
        self.assertIn("kMaxWebConcurrentRequests", config)

        source = _web_binding_source()
        body = _function_body(source, "startRequest", ret="std::shared_ptr<WebTask>")
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
        source = _web_binding_source()
        body = _function_body(source, "applyOptions", ret="void")
        for helper in _REQUEST_BODY_APPLY_HELPERS:
            with self.subTest(helper=helper):
                self.assertIn(
                    helper,
                    body,
                    "applyOptions must delegate request bodies to shared apply* helpers",
                )
                helper_body = _function_body(source, helper, ret="bool")
                self.assertTrue(
                    any(check in helper_body for check in _REQUEST_BODY_CAP_CHECKS),
                    f"{helper} must enforce kMaxWebRequestBytes",
                )

    def test_request_body_methods_enforce_size_cap(self) -> None:
        source = _web_binding_source()
        for method in _REQUEST_BODY_METHODS:
            with self.subTest(method=method):
                body = _function_body(source, method)
                if method in _REQUEST_BODY_APPLY_HELPERS or method.startswith("requestBody"):
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
                        helper_body = _function_body(source, helper, ret="bool")
                        self.assertTrue(
                            any(check in helper_body for check in _REQUEST_BODY_CAP_CHECKS),
                            f"{helper} must enforce kMaxWebRequestBytes",
                        )
                        continue
                self.assertTrue(
                    any(check in body for check in _REQUEST_BODY_CAP_CHECKS),
                    f"{method} must enforce kMaxWebRequestBytes",
                )

    def test_multipart_file_methods_enforce_cumulative_body_cap(self) -> None:
        source = _web_binding_source()
        param_body = _function_body(source, "multipartParam")
        self.assertIn(
            "getBody().size()",
            param_body,
            "multipartParam is the canonical cumulative cap pattern",
        )
        for method in _MULTIPART_CUMULATIVE_BODY_METHODS:
            with self.subTest(method=method):
                body = _function_body(source, method)
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

    def test_off_thread_request_intercept_warns_before_skip(self) -> None:
        source = _read_repo_file("src/bindings/geode/web/GeodeWebListeners.cpp")
        body = _function_body(source, "invokeRequestEvent", ret="bool")
        self.assertIn(
            "!Runtime::isMainThread()",
            body,
            "invokeRequestEvent must detect off-thread intercept events",
        )
        self.assertIn(
            "off-thread intercept skipped",
            body,
            "off-thread intercept skip must log a one-time warning",
        )
        self.assertIn(
            "loggedOffThreadSkip",
            body,
            "off-thread intercept warning must only fire once",
        )

    def test_off_thread_response_listener_is_side_effects_only(self) -> None:
        source = _read_repo_file("src/bindings/geode/web/GeodeWebListeners.cpp")
        body = _function_body(source, "invokeResponseEvent", ret="bool")
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
            "kListenerDescriptors",
            source,
            "web listener registration should use a descriptor table",
        )
        body = _function_body(source, "registerListenerFromDescriptor")
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
        for fn in _WEB_LISTENER_REGISTRARS:
            with self.subTest(fn=fn):
                wrapper = _function_body(source, fn)
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


class WebSocketGuardTests(unittest.TestCase):
    def test_websocket_constants_are_declared(self) -> None:
        config = _read_repo_file(_CONFIG_HEADER)
        for name in (
            "kMaxWebSocketConnections",
            "kMaxWebSocketServers",
            "kMaxWebSocketServerClients",
            "kMaxWebSocketSendBytes",
            "kMaxWebSocketReceiveBytes",
        ):
            with self.subTest(name=name):
                self.assertIn(name, config)

    def test_ws_connect_enforces_connection_cap(self) -> None:
        source = _read_repo_file(_WEBSOCKET_CONNECTION)
        body = _function_body(source, "wsConnect")
        self.assertIn(
            "kMaxWebSocketConnections",
            body,
            "wsConnect must enforce kMaxWebSocketConnections",
        )

    def test_ws_serve_enforces_server_cap_and_client_limit(self) -> None:
        source = _read_repo_file(_WEBSOCKET_SERVER)
        body = _function_body(source, "wsServe")
        self.assertIn(
            "kMaxWebSocketServers",
            body,
            "wsServe must enforce kMaxWebSocketServers",
        )
        self.assertIn(
            "kMaxWebSocketServerClients",
            body,
            "wsServe must pass kMaxWebSocketServerClients to server construction",
        )

    def test_websocket_send_methods_enforce_size_cap(self) -> None:
        internal = _read_repo_file(_WEBSOCKET_INTERNAL)
        self.assertIn("kMaxWebSocketSendBytes", internal)
        self.assertIn("wsSendWithinLimit", internal)

        for rel_path, fn in (
            (_WEBSOCKET_CONNECTION, "sendData"),
            (_WEBSOCKET_SERVER, "broadcastData"),
            (_WEBSOCKET_SERVER, "peerSendData"),
        ):
            with self.subTest(fn=fn):
                source = _read_repo_file(rel_path)
                body = _function_body(source, fn)
                self.assertIn(
                    "wsSendWithinLimit",
                    body,
                    f"{fn} must enforce send size via wsSendWithinLimit",
                )

    def test_websocket_peer_send_delegates_to_peer_send_data(self) -> None:
        source = _read_repo_file(_WEBSOCKET_SERVER)
        for fn in ("peerSend", "peerSendBinary"):
            with self.subTest(fn=fn):
                body = _function_body(source, fn)
                self.assertIn(
                    "peerSendData",
                    body,
                    f"{fn} must delegate to peerSendData for size enforcement",
                )

    def test_websocket_message_callbacks_enforce_receive_cap(self) -> None:
        conn_source = _read_repo_file(_WEBSOCKET_CONNECTION)
        conn_body = _function_body(conn_source, "installMessageCallback", ret="void")
        self.assertIn(
            "kMaxWebSocketReceiveBytes",
            conn_body,
            "client message callback must enforce kMaxWebSocketReceiveBytes",
        )

        server_source = _read_repo_file(_WEBSOCKET_SERVER)
        server_body = _function_body(server_source, "handleServerClientMessage", ret="void")
        self.assertIn(
            "kMaxWebSocketReceiveBytes",
            server_body,
            "server message callback must enforce kMaxWebSocketReceiveBytes",
        )

    def test_websocket_callbacks_queue_to_main_thread(self) -> None:
        internal = _read_repo_file(_WEBSOCKET_INTERNAL)
        start = internal.find("void queueWsEvent(")
        self.assertNotEqual(start, -1, "missing shared queueWsEvent helper")
        end = internal.find("inline constexpr char kWsConnectionClosedMsg", start)
        template_body = internal[start:end]
        self.assertIn(
            "geode::queueInMainThread",
            template_body,
            "queueWsEvent must defer websocket callbacks to the main thread",
        )
        self.assertIn(
            "stopped.load()",
            template_body,
            "queueWsEvent must drop events for stopped owners",
        )

        conn_source = _read_repo_file(_WEBSOCKET_CONNECTION)
        open_body = _function_body(conn_source, "queueOpenEvent", ret="void")
        self.assertIn(
            "queueInMainThread",
            open_body,
            "queueOpenEvent must defer to the main thread",
        )
        self.assertGreaterEqual(
            conn_source.count("queueWsEvent<"),
            3,
            "connection message/close/error events must route through queueWsEvent",
        )

        server_source = _read_repo_file(_WEBSOCKET_SERVER)
        self.assertGreaterEqual(
            server_source.count("queueWsEvent<"),
            4,
            "server connect/message/disconnect/error events must route through queueWsEvent",
        )

    def test_websocket_shutdown_hook_clears_state(self) -> None:
        source = _read_repo_file(_WEBSOCKET_INTERNAL)
        self.assertIn("clearWsState", source)
        self.assertIn("WeakHandlePool", source)
        self.assertIn("ensureShutdownHook(wsShutdownHookRegistered()", source)
        body = _inline_function_body(source, "inline void clearWsState()")
        self.assertIn("activeWsConnections().clearAll", body)
        self.assertIn("activeWsServers().clearAll", body)

    def test_web_shutdown_hook_clears_state(self) -> None:
        source = _read_repo_file(_WEB_INTERNAL)
        self.assertIn("clearWebState", source)
        self.assertIn("WeakHandlePool", source)
        self.assertIn("ensureShutdownHook(webShutdownHookRegistered()", source)
        body = _inline_function_body(source, "inline void clearWebState()")
        self.assertIn("activeTasks().clearAll", body)
        self.assertIn("activeListeners().clearAll", body)


class Render3DGuardTests(unittest.TestCase):
    def test_renderer3d_shutdown_hook_destroys_gl_resources(self) -> None:
        lifetime = _read_repo_file(_RENDERER3D_LIFETIME)
        self.assertIn("destroyGlResources", lifetime)
        self.assertIn("ensureShutdownHook(renderer3DShutdownHookRegistered()", lifetime)
        clear_body = _function_body(lifetime, "clearRenderer3DGlState", ret="void")
        self.assertIn("Renderer3D::instance().destroyGlResources()", clear_body)

        renderer = _read_repo_file(_RENDERER3D)
        self.assertGreaterEqual(
            renderer.count("ensureRenderer3DShutdownHook()"),
            3,
            "Renderer3D GPU entry points must register the shutdown hook on first use",
        )

    def test_ensure_gpu_mesh_does_not_retain_failed_uploads(self) -> None:
        source = _read_repo_file(_RENDERER3D_MESH_CACHE)
        body = _function_body(source, "Renderer3DMeshCache::ensureGpuMesh", ret="GpuMesh*")
        self.assertIn("hasDrawableGpuPrimitive", body)
        self.assertIn("m_gpuMeshes.erase(meshId)", body)
        self.assertIn("return nullptr", body)

    def test_render3d_gl_paths_guard_missing_context(self) -> None:
        mesh_cache = _read_repo_file(_RENDERER3D_MESH_CACHE)
        delete_mesh_body = _function_body(
            mesh_cache, "Renderer3DMeshCache::deleteGpuMesh", ret="void"
        )
        self.assertIn("glContextAvailable()", delete_mesh_body)

        ensure_mesh_body = _function_body(
            mesh_cache, "Renderer3DMeshCache::ensureGpuMesh", ret="GpuMesh*"
        )
        self.assertIn("glContextAvailable()", ensure_mesh_body)

        ensure_tex_body = _function_body(
            mesh_cache, "Renderer3DMeshCache::ensureGpuTexture", ret="unsigned int"
        )
        self.assertIn("glContextAvailable()", ensure_tex_body)

        texture_body = _function_body(
            _read_repo_file(_RENDERER3D_TEXTURE2D), "uploadRgbaTexture2D", ret="unsigned int"
        )
        self.assertIn("glContextAvailable()", texture_body)

        delete_vao_body = _function_body(
            _read_repo_file(_RENDERER3D_GL_UTIL), "deleteVao", ret="void"
        )
        self.assertIn("glContextAvailable()", delete_vao_body)

    def test_viewport_frame_destructor_releases_texture_registry(self) -> None:
        source = _read_repo_file(_CC_VIEWPORT_FRAME)
        dtor_match = re.search(
            r"CCViewportFrame::~CCViewportFrame\(\)\s*\{([^}]+)\}",
            source,
            re.DOTALL,
        )
        assert dtor_match is not None, "CCViewportFrame destructor must exist"
        dtor_body = dtor_match.group(1)
        self.assertIn(
            "releaseViewportTexture()",
            dtor_body,
            "viewport destruction must release TextureRegistry entries",
        )
        release_body = _function_body(source, "CCViewportFrame::releaseViewportTexture", ret="void")
        self.assertIn("TextureRegistry::instance().release", release_body)
        self.assertIn("Renderer3D::instance().releaseTextureGpu", release_body)
        self.assertIn("m_viewportTextureId = 0", release_body)


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
        self.assertIn("logCallbackFailure", source)
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
                    "logCallbackFailure",
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
    def test_scheduled_handle_binding_cancels_via_scheduler(self) -> None:
        source = _read_repo_file(_SCHEDULED_HANDLE_BINDING)
        self.assertIn("Scheduler::get().cancel", source)
        self.assertIn("lua_newuserdatataggedwithmetatable", source)
        self.assertIn("registerTaggedMetatable", source)

    def test_task_handle_metatable_registers_gc_cancellation(self) -> None:
        source = _read_repo_file(_TASK_BINDING)
        self.assertIn("ScheduledHandleBinding", source)
        self.assertIn("detail::taskHandleTag()", source)
        self.assertIn("TaskHandleBinding::registerMetatable", source)

    def test_imgui_handle_metatable_registers_gc_cancellation(self) -> None:
        source = _read_repo_file(_IMGUI_BINDING)
        self.assertIn("ScheduledHandleBinding", source)
        self.assertIn("detail::imguiDrawHandleTag()", source)
        self.assertIn("ImGuiDrawHandleBinding::registerMetatable", source)

    def test_schedulers_fire_callbacks_via_framework_helper(self) -> None:
        callback_source = _read_repo_file(_SCHEDULED_CALLBACK)
        self.assertIn("fireProtectedCallback", callback_source)
        self.assertIn("protectedCall", callback_source)

        task_scheduler = _read_repo_file(_TASK_SCHEDULER)
        self.assertIn("fireProtectedCallback", task_scheduler)

        imgui_scheduler = _read_repo_file("src/bindings/imgui/ImGuiDrawScheduler.cpp")
        self.assertIn("fireProtectedCallback", imgui_scheduler)

    def test_schedulers_store_slots_via_framework_mixin(self) -> None:
        store_source = _read_repo_file(_SCHEDULED_SLOT_STORE)
        self.assertIn("ScheduledSlotStore", store_source)
        self.assertIn("compactCancelled", store_source)

        task_scheduler = _read_repo_file(_TASK_SCHEDULER)
        self.assertIn("ScheduledSlotStore", task_scheduler)

        imgui_scheduler = _read_repo_file("src/bindings/imgui/ImGuiDrawScheduler.cpp")
        self.assertIn("m_store", imgui_scheduler)
        self.assertIn("compactCancelled", imgui_scheduler)


class ImGuiGuardTests(unittest.TestCase):
    def test_host_tests_use_imgui_host_stub(self) -> None:
        cmake = _read_repo_file(_CMAKE_LISTS)
        self.assertIn("tests/host/ImGuiHostStub.cpp", cmake)
        self.assertNotIn("src/bindings/imgui/ImGuiHost.cpp", cmake)

        stub = _read_repo_file(_IMGUI_HOST_STUB)
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
        source = _read_repo_file(_IMGUI_BINDING)
        self.assertIn("#if !defined(LUAUAPI_HOST_TESTS)", source)
        self.assertIn("initImGuiHost();", source)

    def test_style_with_uses_raii_pop_guards(self) -> None:
        source = _read_repo_file(_IMGUI_STYLE)
        self.assertIn("ImGuiStyleVarPopGuard", source)
        self.assertIn("ImGuiStyleColorPopGuard", source)
        body = _function_body(source, "imguiStyleWith")
        self.assertIn("callDrawClosure", body)

    def test_scoped_imgui_wrappers_use_end_guards(self) -> None:
        scoped_sources = (
            _IMGUI_BINDING_CPP,
            "src/bindings/imgui/ImGuiPopups.cpp",
            "src/bindings/imgui/ImGuiTables.cpp",
            "src/bindings/imgui/ImGuiMenus.cpp",
            "src/bindings/imgui/ImGuiLayout.cpp",
        )
        for source_path in scoped_sources:
            with self.subTest(source=source_path):
                source = _read_repo_file(source_path)
                self.assertTrue(
                    "ImGuiEndGuard" in source or "ImGuiConditionalEndGuard" in source,
                    f"{source_path} must use scoped end guards",
                )

    def test_imgui_binding_tests_cover_headless_checks(self) -> None:
        source = _read_repo_file(_IMGUI_BINDING_TESTS)
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
        cmake = _read_repo_file(_CMAKE_LISTS)
        headless = _read_repo_file(_IMGUI_HEADLESS_CMAKE)
        self.assertIn("cmake/ImGuiHeadless.cmake", cmake)
        self.assertIn("luauapi_imgui_headless", cmake)
        self.assertIn("imgui_impl_null.cpp", headless)
        self.assertIn("host/ImGuiTestHarness.hpp", _read_repo_file(_IMGUI_BINDING_TESTS))
        for source in (
            "src/bindings/imgui/ImGuiBinding.cpp",
            "src/bindings/imgui/ImGuiConstants.cpp",
            "src/bindings/imgui/ImGuiStyle.cpp",
            "src/bindings/imgui/ImGuiWidgets.cpp",
        ):
            with self.subTest(source=source):
                self.assertIn(source, cmake)


class ErrorSemanticsGuardTests(unittest.TestCase):
    def test_usertype_field_get_raises_on_failure(self) -> None:
        source = _read_repo_file(_USERTYPE)
        index_body = _function_body(source, "usertypeIndex")
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
        source = _read_repo_file(_FS_BINDING)
        body = _function_body(source, "fsExists")
        self.assertIn("if (ec)", body)
        self.assertIn("return 2", body)

    def test_json_convert_reports_depth_overflow(self) -> None:
        source = _read_repo_file(_JSON_CONVERT)
        self.assertIn("kJsonDepthExceededMsg", source)
        self.assertIn("geode::Result<void> pushJson", source)
        self.assertIn("geode::Result<matjson::Value> toJson", source)

    def test_mod_set_saved_value_checks_conversion_and_container(self) -> None:
        source = _read_repo_file(_GEODE_MOD)
        body = _function_body(source, "modSetSavedValue")
        self.assertIn("valueResult.isErr()", body)
        self.assertIn("isObject()", body)
        self.assertGreaterEqual(body.count("pushNilErr"), 2)

    def test_delegate_default_return_policy_documented(self) -> None:
        source = _read_repo_file("docs/reference/lua/delegates.md")
        self.assertIn("logs the failure", source)
        self.assertIn("method default", source)


class FreeFnManifestSyncTests(unittest.TestCase):
    def test_scanner_and_emitter_share_one_manifest(self) -> None:
        from luau_codegen.model.free_fn_sources import free_function_includes

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
