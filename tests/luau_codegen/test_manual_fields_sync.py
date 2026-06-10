from __future__ import annotations

import os
import re
import unittest

from luau_codegen.emit.luau_types.manual_fields import (  # type: ignore[import-unresolved]
    MANUAL_FREE_FN_FIELDS,
)

_REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

_NAMESPACE_SOURCES = {
    "geode.cocos": "src/bindings/geode/GeodeCocosBinding.cpp",
    "geode.utils.base64": "src/bindings/geode/GeodeBase64Binding.cpp",
    "geode.utils.permission": "src/bindings/geode/GeodePermissionBinding.cpp",
    "geode.ColorProvider": "src/bindings/geode/GeodeColorProviderBinding.cpp",
    "geode.VersionInfo": "src/bindings/geode/GeodeVersionBinding.cpp",
    "geode.Keybind": "src/bindings/geode/GeodeKeybindBinding.cpp",
}

_SET_CFUNCTION = re.compile(r'setTableCFunction\(L,\s*[^,]+,\s*"([^"]+)"')
_SET_FIELD = re.compile(r'lua_setfield\(L,\s*[^,]+,\s*"([^"]+)"\)')


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
    return set(_SET_CFUNCTION.findall(body)) | set(_SET_FIELD.findall(body))


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


if __name__ == "__main__":
    unittest.main()
