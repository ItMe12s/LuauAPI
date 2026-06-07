from __future__ import annotations

import os
import re
import unittest

_REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

_EXTRA_BINDING_SOURCES = {
    "fs": {
        "dluau": "tools/luau_codegen/extra_bindings/fs.dluau",
        "cpp": "src/lua/bindings/geode/GeodeFsBinding.cpp",
        "type_name": "FsNamespace",
        "start_marker": "registerGeodeFs(lua_State* L)",
        "end_marker": 'lua_setfield(L, -2, "fs")',
    },
    "json": {
        "dluau": "tools/luau_codegen/extra_bindings/json.dluau",
        "cpp": "src/lua/bindings/geode/GeodeJsonBinding.cpp",
        "type_name": "JsonNamespace",
        "start_marker": "registerGeodeJson(lua_State* L)",
        "end_marker": 'lua_setfield(L, -2, "json")',
    },
    "mod": {
        "dluau": "tools/luau_codegen/extra_bindings/mod.dluau",
        "cpp": "src/lua/bindings/geode/GeodeModBinding.cpp",
        "type_name": "ModNamespace",
        "start_marker": "registerGeodeMod(lua_State* L)",
        "end_marker": 'lua_setfield(L, -2, "Mod")',
    },
    "web": {
        "dluau": "tools/luau_codegen/extra_bindings/web.dluau",
        "cpp": "src/lua/bindings/geode/GeodeWebBinding.cpp",
        "type_name": "WebNamespace",
        "start_marker": "registerGeodeWeb(lua_State* L)",
        "end_marker": "registerConstants(L)",
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
