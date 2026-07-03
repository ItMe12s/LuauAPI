from __future__ import annotations

import os
import re

_REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

WEB_BINDING_SOURCES = (
    "src/bindings/geode/web/GeodeWebCore.cpp",
    "src/bindings/geode/web/GeodeWebApi.cpp",
    "src/bindings/geode/web/GeodeWebListeners.cpp",
)
WEB_INTERNAL = "src/bindings/geode/web/WebInternal.hpp"
FS_BINDING = "src/bindings/geode/GeodeFsBinding.cpp"
WEBSOCKET_CONNECTION = "src/bindings/websocket/WebSocketConnection.cpp"
WEBSOCKET_SERVER = "src/bindings/websocket/WebSocketServer.cpp"
WEBSOCKET_INTERNAL = "src/bindings/websocket/WebSocketInternal.hpp"
RENDERER3D_LIFETIME = "src/render3d/gpu/Renderer3DResourceLifetime.cpp"
RENDERER3D = "src/render3d/gpu/Renderer3D.cpp"
RENDERER3D_MESH_CACHE = "src/render3d/gpu/Renderer3DMeshCache.cpp"
RENDERER3D_GL_UTIL = "src/render3d/gpu/GlUtil.cpp"
RENDERER3D_TEXTURE2D = "src/render3d/gpu/Texture2D.cpp"
CC_VIEWPORT_FRAME = "src/render3d/viewport/CCViewportFrame.cpp"
COCOS_BINDING = "src/bindings/geode/GeodeCocosBinding.cpp"
TASK_SCHEDULER = "src/bindings/task/TaskScheduler.cpp"
TASK_BINDING = "src/bindings/task/TaskBinding.cpp"
IMGUI_BINDING = "src/bindings/imgui/ImGuiCore.cpp"
IMGUI_HOST_STUB = "tests/host/ImGuiHostStub.cpp"
IMGUI_STYLE = "src/bindings/imgui/ImGuiStyleFonts.cpp"
IMGUI_BINDING_TESTS = "tests/cpp/bindings/imgui_binding_tests.cpp"
IMGUI_HEADLESS_CMAKE = "cmake/ImGuiHeadless.cmake"
CMAKE_LISTS = "CMakeLists.txt"
TEST_SOURCES_CMAKE = "cmake/TestSources.cmake"
SCHEDULED_HANDLE_BINDING = "src/framework/schedule/ScheduledHandleBinding.hpp"
CANCELLABLE_SLOTS = "src/framework/schedule/CancellableSlots.hpp"
LUA_COCOS = "src/framework/callback/LuaCocosHandler.cpp"
LUA_DELEGATE = "src/framework/callback/LuaDelegate.cpp"
LUA_CALLBACK = "src/framework/callback/LuaCallback.hpp"
USERTYPE = "src/framework/usertype/Usertype.cpp"
JSON_CONVERT = "src/bindings/geode/JsonConvert.cpp"
GEODE_MOD = "src/bindings/geode/GeodeModBinding.cpp"
GEODE_PERMISSION = "src/bindings/geode/GeodeSmallBindings.cpp"
CONFIG_HEADER = "src/core/Config.hpp"
MAIN_CPP = "src/main.cpp"

COCOS_GENERATED_OWNERS = frozenset(
    {
        "getObjectName",
        "handleTouchPriority",
        "handleTouchPriorityWith",
    }
)

WEB_LISTENER_REGISTRARS = (
    "webOnRequestIntercept",
    "webOnRequestInterceptFor",
    "webOnRequestInterceptById",
    "webOnResponse",
    "webOnResponseFor",
    "webOnResponseById",
)

REQUEST_BODY_APPLY_HELPERS = (
    "applyBody",
    "applyBodyString",
    "applyBodyJson",
    "applyBodyMultipart",
)

REQUEST_BODY_METHODS = (
    "requestBody",
    "requestBodyString",
    "requestBodyJson",
    "requestBodyMultipart",
    "multipartParam",
    "multipartFile",
    "multipartFileFrom",
    "multipartGetBody",
)

REQUEST_BODY_CAP_CHECKS = (
    "requestBodyWithinLimit",
    "requestJsonBodyWithinLimit",
)

MULTIPART_CUMULATIVE_BODY_METHODS = (
    "multipartFile",
    "multipartFileFrom",
)


def web_binding_source() -> str:
    return "\n".join(read_repo_file(path) for path in WEB_BINDING_SOURCES)


def read_repo_file(rel_path: str) -> str:
    path = os.path.join(_REPO_ROOT, rel_path)
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def function_body(source: str, name: str, *, ret: str = "int") -> str:
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


def assert_gl_context_guard(body: str, *, fn: str) -> None:
    assert "glContextAvailable()" in body or "canDeleteGpuResources(" in body, (
        f"{fn} must guard missing GL context"
    )


def inline_function_body(source: str, signature: str) -> str:
    start = source.find(signature)
    assert start != -1, f"missing inline function {signature!r}"
    next_inline = source.find("\n    inline ", start + len(signature))
    next_namespace = source.find("\n} // namespace", start + len(signature))
    candidates = [pos for pos in (next_inline, next_namespace) if pos != -1]
    end = min(candidates) if candidates else len(source)
    return source[start:end]


def registered_cocos_fields() -> set[str]:
    source = read_repo_file(COCOS_BINDING)
    marker = 'getOrCreateTable(L, "geode.cocos")'
    start = source.find(marker)
    assert start != -1, "missing geode.cocos registration block"
    end = source.find("return geode::Ok();", start)
    body = source[start:end]
    names = set(re.findall(r'setTableCFunction\(L,\s*[^,]+,\s*"([^"]+)"', body))
    names.update(re.findall(r'lua_setfield\(L,\s*[^,]+,\s*"([^"]+)"\)', body))
    names.update(re.findall(r'registerIntEnumTable\(L,\s*[^,]+,\s*"([^"]+)"', body))
    return names


def task_scheduler_production_branch() -> str:
    source = read_repo_file(TASK_SCHEDULER)
    host_guard = "#if !defined(LUAUAPI_HOST_TESTS)"
    start = source.find(host_guard)
    assert start != -1, "missing host-test guard in TaskScheduler.cpp"
    end = source.find("#else", start)
    assert end != -1, "missing host-test else branch"
    return source[start:end]
