from __future__ import annotations

import os
import re
import unittest

_REPO_ROOT = os.path.dirname(
    os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
)

_PUBLIC_HEADER = "include/LuauAPI.hpp"
_HOST_HEADER = "tests/host/include/LuauAPI.hpp"

_HOST_INTENTIONAL_OMISSIONS = {
    "runFileAsync",
    "runScriptAsync",
}

_EXPORTED_FN = re.compile(
    r"(?:LUAUAPI_DLL\s+)?"
    r"(?:arc::Future<geode::Result<void>>|geode::Result<void>|bool|RuntimeStatus|std::(?:string|size_t))\s+"
    r"(\w+)\s*\(",
)


def _read_repo_file(rel_path: str) -> str:
    path = os.path.join(_REPO_ROOT, rel_path)
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def _namespace_body(header_source: str) -> str:
    start = header_source.find("namespace imes::luauapi")
    assert start != -1, "missing imes::luauapi namespace"
    open_brace = header_source.find("{", start)
    assert open_brace != -1, "missing namespace open brace"
    depth = 0
    for index in range(open_brace, len(header_source)):
        char = header_source[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return header_source[open_brace + 1 : index]
    raise AssertionError("unterminated imes::luauapi namespace")


def _exported_symbols(header_source: str) -> set[str]:
    collapsed = re.sub(r"\s+", " ", _namespace_body(header_source))
    return set(_EXPORTED_FN.findall(collapsed))


class PublicApiHeaderSyncTests(unittest.TestCase):
    def test_host_header_matches_public_api_minus_intentional_omissions(self) -> None:
        public_symbols = _exported_symbols(_read_repo_file(_PUBLIC_HEADER))
        host_symbols = _exported_symbols(_read_repo_file(_HOST_HEADER))

        expected_host = public_symbols - _HOST_INTENTIONAL_OMISSIONS
        missing_from_host = expected_host - host_symbols
        extra_in_host = host_symbols - public_symbols

        self.assertFalse(
            missing_from_host,
            f"host LuauAPI.hpp missing exported symbols: {sorted(missing_from_host)}",
        )
        self.assertFalse(
            extra_in_host,
            f"host LuauAPI.hpp has unexpected symbols: {sorted(extra_in_host)}",
        )

    def test_public_async_entry_points_stay_absent_from_host_header(self) -> None:
        host_symbols = _exported_symbols(_read_repo_file(_HOST_HEADER))
        self.assertFalse(host_symbols & _HOST_INTENTIONAL_OMISSIONS)


if __name__ == "__main__":
    unittest.main()
