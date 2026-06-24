from __future__ import annotations

import json
import os
import re
import tempfile
import unittest

import test_support
from luau_codegen.emit.luau_types import emit as emit_luau_types  # type: ignore[import-unresolved]
from luau_codegen.emit.luau_types.manual_fields import (  # type: ignore[import-unresolved]
    MANUAL_FREE_FN_FIELDS,
)
from luau_codegen.emit.metadata import emit_schema  # type: ignore[import-unresolved]
from luau_codegen.emit.plan import collect_plan  # type: ignore[import-unresolved]
from luau_codegen.parse.broma import Class, Field, Root  # type: ignore[import-unresolved]

_REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
_EXTRA_BINDINGS_DIR = os.path.join(_REPO_ROOT, "tools", "luau_codegen", "extra_bindings")

_NAMESPACE_SOURCES = {
    "geode.cocos": "src/bindings/geode/GeodeCocosBinding.cpp",
    "geode.utils.base64": "src/bindings/geode/GeodeSmallBindings.cpp",
    "geode.utils.permission": "src/bindings/geode/GeodeSmallBindings.cpp",
    "geode.ColorProvider": "src/bindings/geode/GeodeSmallBindings.cpp",
    "geode.VersionInfo": "src/bindings/geode/GeodeSmallBindings.cpp",
    "geode.Keybind": "src/bindings/geode/GeodeSmallBindings.cpp",
    "geode.KeyboardModifier": "src/bindings/geode/GeodeKeyboardBinding.cpp",
    "geode.KeyboardInputData": "src/bindings/geode/GeodeKeyboardBinding.cpp",
}

_SET_CFUNCTION = re.compile(r'setTableCFunction\(L,\s*[^,]+,\s*"([^"]+)"')
_SET_FIELD = re.compile(r'lua_setfield\(L,\s*[^,]+,\s*"([^"]+)"\)')
_SET_INT_ENUM_TABLE = re.compile(r'registerIntEnumTable\(L,\s*[^,]+,\s*"([^"]+)"')


def _read_repo_file(rel_path: str) -> str:
    path = os.path.join(_REPO_ROOT, rel_path)
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def _register_body(source: str, namespace: str) -> str:
    marker = f'getOrCreateTable(L, "{namespace}")'
    start = source.find(marker)
    assert start != -1, f"no getOrCreateTable for {namespace}"
    end = source.find("return geode::Ok();", start)
    assert end != -1, f"no register return for {namespace}"
    return source[start:end]


def _registered_fields(namespace: str) -> set[str]:
    path = os.path.join(_REPO_ROOT, _NAMESPACE_SOURCES[namespace])
    with open(path, "r", encoding="utf-8") as f:
        body = _register_body(f.read(), namespace)
    return (
        set(_SET_CFUNCTION.findall(body))
        | set(_SET_FIELD.findall(body))
        | set(_SET_INT_ENUM_TABLE.findall(body))
    )


def _declared_fields(namespace: str) -> set[str]:
    return {entry.split(":", 1)[0].strip() for entry in MANUAL_FREE_FN_FIELDS[namespace]}


class ManualFieldsSyncTests(unittest.TestCase):
    def test_namespaces_have_declarations(self) -> None:
        for namespace in _NAMESPACE_SOURCES:
            self.assertIn(
                namespace,
                MANUAL_FREE_FN_FIELDS,
                f"{namespace} has a C++ binding but no manual_fields entry",
            )

    def test_cpp_registration_matches_manual_fields(self) -> None:
        for namespace in _NAMESPACE_SOURCES:
            with self.subTest(namespace=namespace):
                registered = _registered_fields(namespace)
                declared = _declared_fields(namespace)
                missing_from_stub = registered - declared
                missing_from_cpp = declared - registered
                self.assertFalse(
                    missing_from_stub,
                    f"{namespace}: registered in C++ but missing from manual_fields.py: "
                    f"{sorted(missing_from_stub)}",
                )
                self.assertFalse(
                    missing_from_cpp,
                    f"{namespace}: declared in manual_fields.py but not registered in C++: "
                    f"{sorted(missing_from_cpp)}",
                )

    def test_emitted_stub_includes_manual_fields(self) -> None:
        root = Root(classes=[Class(name="CCObject", namespace="cocos2d")])
        text = emit_luau_types(root, manual_fields=MANUAL_FREE_FN_FIELDS)["geode.d.luau"]
        for namespace, fields in MANUAL_FREE_FN_FIELDS.items():
            with self.subTest(namespace=namespace):
                for field in fields:
                    self.assertIn(
                        field.strip(),
                        text,
                        f"{namespace}: manual field missing from emitted stub: {field}",
                    )

    def test_geode_utils_web_bridge_manual_field(self) -> None:
        self.assertIn("web: WebNamespace", MANUAL_FREE_FN_FIELDS["geode.utils"])
        cpp = _read_repo_file("src/bindings/geode/web/GeodeWebCore.cpp")
        self.assertIn('getOrCreateTable(L, "geode.utils.web")', cpp)

        root = Root(classes=[Class(name="CCObject", namespace="cocos2d")])
        text = emit_luau_types(root, manual_fields=MANUAL_FREE_FN_FIELDS)["geode.d.luau"]
        utils_start = text.index("utils: {")
        utils_end = text.index("},\n", utils_start)
        utils = text[utils_start:utils_end]
        self.assertIn("web: WebNamespace", utils)
        self.assertNotRegex(utils, r"web:\s*\{[^}]*newRequest:")

    def test_schema_includes_manual_and_extra_binding_provenance(self) -> None:
        root = Root(classes=[Class(name="CCObject", namespace="cocos2d")])
        plan = collect_plan(root, "win")
        with tempfile.TemporaryDirectory() as tmp:
            schema_path = os.path.join(tmp, "schema.json")
            emit_schema(root, schema_path, plan)
            with open(schema_path, "r", encoding="utf-8") as f:
                schema = json.load(f)
        self.assertEqual(schema["manualFields"], MANUAL_FREE_FN_FIELDS)
        expected_extra = sorted(
            os.path.join("tools", "luau_codegen", "extra_bindings", name)
            for name in os.listdir(_EXTRA_BINDINGS_DIR)
            if name.endswith(".dluau")
        )
        self.assertEqual(schema["extraBindings"], expected_extra)
        self.assertIn("unscannedGdEnums", schema)
        self.assertIsInstance(schema["unscannedGdEnums"], list)


if __name__ == "__main__":
    unittest.main()
