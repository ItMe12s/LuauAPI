from __future__ import annotations

import json
import os
import sys
from pathlib import Path

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TOOLS_DIR = os.path.join(ROOT, "tools")
if TOOLS_DIR not in sys.path:
    sys.path.insert(0, TOOLS_DIR)

from luau_codegen.emit.delegates import (  # type: ignore[import-unresolved]
    DELEGATE_SPECS_MODULE,
    collect as collect_delegate_specs,
    fallback_bindings_dir,
    install_delegate_specs_module,
)
from luau_codegen.emit.luau_types import TYPES_FILE  # type: ignore[import-unresolved]
from luau_codegen.emit.value_struct_specs import (  # type: ignore[import-unresolved]
    VALUE_STRUCT_SPECS_MODULE,
    collect_value_struct_specs,
    install_value_struct_specs_module,
)
from luau_codegen.parse.collect import collect_bindings_root  # type: ignore[import-unresolved]


def _install_fixture_delegate_specs() -> None:
    fixture = fallback_bindings_dir()
    if not fixture.is_dir():
        return
    install_delegate_specs_module(
        collect_delegate_specs(fixture),
        specs_path=fixture / "_fixture_delegate_specs.py",
        module_name=DELEGATE_SPECS_MODULE,
    )


def resolve_test_bindings_dir(
    deps: tuple[str, ...] = ("geode_bindings-src", "bindings-src", "bindings-audit"),
) -> str | None:
    env = os.environ.get("LUAUAPI_BINDINGS_DIR")
    if env and os.path.isfile(os.path.join(env, "GeometryDash.bro")):
        return env

    version = "2.2081"
    mod_json = os.path.join(ROOT, "mod.json")
    try:
        with open(mod_json, encoding="utf-8") as f:
            version = json.load(f).get("gd", {}).get("win", version)
    except (OSError, ValueError):
        pass

    for dep in deps:
        candidate = os.path.join(ROOT, "build", "_deps", dep, "bindings", version)
        if os.path.isfile(os.path.join(candidate, "GeometryDash.bro")):
            return candidate
    return None


def _install_fixture_value_struct_specs() -> None:
    bindings = resolve_test_bindings_dir()
    if not bindings:
        return
    root = collect_bindings_root(bindings)
    specs = collect_value_struct_specs(root)
    install_value_struct_specs_module(
        specs,
        specs_path=Path(bindings) / "_fixture_value_struct_specs.py",
        module_name=VALUE_STRUCT_SPECS_MODULE,
        preserve_existing_on_empty=True,
    )


_install_fixture_delegate_specs()
_install_fixture_value_struct_specs()
DELEGATE_SPECS = sys.modules[DELEGATE_SPECS_MODULE].DELEGATE_SPECS


def reinstall_fixture_value_struct_specs() -> None:
    _install_fixture_value_struct_specs()


def all_platforms(value: str = "0x1") -> dict[str, str]:
    return {
        "win": value,
        "m1": value,
        "ios": value,
        "android32": value,
        "android64": value,
    }


def types_text(files: dict[str, str]) -> str:
    return files[TYPES_FILE]
