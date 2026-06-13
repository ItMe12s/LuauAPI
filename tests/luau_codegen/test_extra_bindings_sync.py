from __future__ import annotations

import glob
import os
import re
import unittest

_REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
_EXTRA_BINDINGS_DIR = os.path.join(_REPO_ROOT, "tools", "luau_codegen", "extra_bindings")
_TYPE_STUBS_DOC = os.path.join(_REPO_ROOT, "docs", "reference", "lua", "type-stubs.md")

_EXTRA_BINDING_SOURCES = {
    "fs": {
        "dluau": "tools/luau_codegen/extra_bindings/fs.dluau",
        "cpp": "src/bindings/geode/GeodeFsBinding.cpp",
        "type_name": "FsNamespace",
        "start_marker": "registerGeodeFs(lua_State* L)",
        "end_marker": 'lua_setfield(L, -2, "fs")',
    },
    "json": {
        "dluau": "tools/luau_codegen/extra_bindings/json.dluau",
        "cpp": "src/bindings/geode/GeodeSmallBindings.cpp",
        "type_name": "JsonNamespace",
        "start_marker": "registerGeodeJson(lua_State* L)",
        "end_marker": 'lua_setfield(L, -2, "json")',
    },
    "mod": {
        "dluau": "tools/luau_codegen/extra_bindings/mod.dluau",
        "cpp": "src/bindings/geode/GeodeModBinding.cpp",
        "type_name": "ModNamespace",
        "start_marker": "registerGeodeMod(lua_State* L)",
        "end_marker": 'lua_setfield(L, -2, "Mod")',
    },
    "web": {
        "dluau": "tools/luau_codegen/extra_bindings/web.dluau",
        "cpp": "src/bindings/geode/web/GeodeWebBinding.cpp",
        "type_name": "WebNamespace",
        "start_marker": "registerGeodeWeb(lua_State* L)",
        "end_marker": "registerConstants(L)",
    },
    "websocket": {
        "dluau": "tools/luau_codegen/extra_bindings/websocket.dluau",
        "cpp": "src/bindings/websocket/WebSocketBinding.cpp",
        "type_name": "WebSocketNamespace",
        "start_marker": "registerWebSocket(lua_State* L)",
        "end_marker": 'lua_setglobal(L, "websocket")',
    },
    "imgui": {
        "dluau": "tools/luau_codegen/extra_bindings/imgui.dluau",
        "cpp": "src/bindings/imgui/ImGuiBinding.cpp",
        "type_name": "ImGuiNamespace",
        "start_marker": "registerImGui(lua_State* L)",
        "end_marker": 'lua_setglobal(L, "imgui")',
    },
    "task": {
        "dluau": "tools/luau_codegen/extra_bindings/task.dluau",
        "cpp": "src/bindings/task/TaskBinding.cpp",
        "type_name": "TaskNamespace",
        "start_marker": "registerTask(lua_State* L)",
        "end_marker": 'lua_setglobal(L, "task")',
    },
    "time": {
        "dluau": "tools/luau_codegen/extra_bindings/task.dluau",
        "cpp": "src/bindings/task/TaskBinding.cpp",
        "type_name": "TimeNamespace",
        "start_marker": 'lua_setglobal(L, "task")',
        "end_marker": 'lua_setglobal(L, "time")',
    },
    "gd3d_transform": {
        "dluau": "tools/luau_codegen/extra_bindings/gd3d.dluau",
        "cpp": "src/bindings/render3d/TransformBinding.cpp",
        "type_name": "Gd3dTransformNamespace",
        "start_marker": 'getOrCreateTable(L, "gd3d.Transform")',
        "end_marker": "return geode::Ok();\n    }\n} // namespace luax",
    },
    "gd3d_material": {
        "dluau": "tools/luau_codegen/extra_bindings/gd3d.dluau",
        "cpp": "src/bindings/render3d/MaterialBinding.cpp",
        "type_name": "Gd3dMaterialNamespace",
        "start_marker": 'getOrCreateTable(L, "gd3d.Material")',
        "end_marker": "return geode::Ok();\n    }\n} // namespace luax",
    },
    "gd3d_texture": {
        "dluau": "tools/luau_codegen/extra_bindings/gd3d.dluau",
        "cpp": "src/bindings/render3d/TextureBinding.cpp",
        "type_name": "Gd3dTextureNamespace",
        "start_marker": 'getOrCreateTable(L, "gd3d.texture")',
        "end_marker": "return geode::Ok();\n    }\n} // namespace luax",
    },
    "gd3d_gltf": {
        "dluau": "tools/luau_codegen/extra_bindings/gd3d.dluau",
        "cpp": "src/bindings/render3d/GltfBinding.cpp",
        "type_name": "Gd3dGltfNamespace",
        "start_marker": 'getOrCreateTable(L, "gd3d.gltf")',
        "end_marker": "return geode::Ok();\n    }\n} // namespace luax",
    },
    "gd3d_mesh": {
        "dluau": "tools/luau_codegen/extra_bindings/gd3d.dluau",
        "cpp": "src/bindings/render3d/ProceduralMeshBinding.cpp",
        "type_name": "Gd3dMeshNamespace",
        "start_marker": 'getOrCreateTable(L, "gd3d.mesh")',
        "end_marker": "return geode::Ok();\n    }\n} // namespace luax",
    },
    "gd3d_viewport": {
        "dluau": "tools/luau_codegen/extra_bindings/gd3d.dluau",
        "cpp": "src/bindings/render3d/ViewportFrameBinding.cpp",
        "type_name": "Gd3dViewportFrameNamespace",
        "start_marker": 'getOrCreateTable(L, "gd3d.ViewportFrame")',
        "end_marker": "return geode::Ok();\n    }\n} // namespace luax",
    },
}

_DECLARED_FN_FIELD = re.compile(r"^\s*(\w+)\s*:\s*\(", re.MULTILINE)
_SET_CFUNCTION = re.compile(r'setTableCFunction\(L,\s*[^,]+,\s*"([^"]+)"')


def _read_repo_file(rel_path: str) -> str:
    path = os.path.join(_REPO_ROOT, rel_path)
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def _declared_namespace_fields(dluau_source: str, type_name: str) -> set[str]:
    marker = f"export type {type_name} = {{"
    start = dluau_source.find(marker)
    assert start != -1, f"no export type {type_name} in dluau source"
    open_brace = start + len(marker) - 1
    depth = 0
    body_start = open_brace + 1
    for index in range(open_brace, len(dluau_source)):
        char = dluau_source[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                body = dluau_source[body_start:index]
                return set(_DECLARED_FN_FIELD.findall(body))
    raise AssertionError(f"unterminated export type {type_name}")


def _registered_namespace_fields(cpp_source: str, start_marker: str, end_marker: str) -> set[str]:
    start = cpp_source.find(start_marker)
    assert start != -1, f"missing start marker {start_marker}"
    end = cpp_source.find(end_marker, start)
    assert end != -1, f"missing end marker {end_marker}"
    body = cpp_source[start:end]
    return set(_SET_CFUNCTION.findall(body))


class ExtraBindingsSyncTests(unittest.TestCase):
    def test_type_stubs_doc_lists_every_extra_binding(self) -> None:
        doc = _read_repo_file(os.path.relpath(_TYPE_STUBS_DOC, _REPO_ROOT))
        dluau_files = sorted(
            os.path.basename(path)
            for path in glob.glob(os.path.join(_EXTRA_BINDINGS_DIR, "*.dluau"))
        )
        missing = [name for name in dluau_files if name not in doc]
        self.assertFalse(
            missing,
            f"type-stubs.md missing extra_bindings entries: {missing}",
        )

    def test_extra_binding_dluau_matches_cpp_registration(self) -> None:
        for binding, spec in _EXTRA_BINDING_SOURCES.items():
            with self.subTest(binding=binding):
                dluau_source = _read_repo_file(spec["dluau"])
                cpp_source = _read_repo_file(spec["cpp"])
                declared = _declared_namespace_fields(dluau_source, spec["type_name"])
                registered = _registered_namespace_fields(
                    cpp_source, spec["start_marker"], spec["end_marker"]
                )
                missing_from_stub = registered - declared
                missing_from_cpp = declared - registered
                self.assertFalse(
                    missing_from_stub,
                    f"{binding}: registered in C++ but missing from dluau: "
                    f"{sorted(missing_from_stub)}",
                )
                self.assertFalse(
                    missing_from_cpp,
                    f"{binding}: declared in dluau but not registered in C++: "
                    f"{sorted(missing_from_cpp)}",
                )


if __name__ == "__main__":
    unittest.main()
