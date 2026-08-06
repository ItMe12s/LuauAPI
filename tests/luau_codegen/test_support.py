from __future__ import annotations

import json
import os
import sys
from functools import lru_cache
from pathlib import Path

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TOOLS_DIR = os.path.join(ROOT, "tools")
if TOOLS_DIR not in sys.path:
    sys.path.insert(0, TOOLS_DIR)

DELEGATE_FIXTURE_DIR = Path(ROOT) / "tests" / "luau_codegen" / "fixtures" / "delegate_bindings"


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


@lru_cache(maxsize=1)
def fixture_codegen_context():
    from luau_codegen.emit.delegates import build_delegate_catalog, collect_delegate_specs
    from luau_codegen.emit.value_struct_specs import collect_value_struct_specs
    from luau_codegen.model.codegen_context import CodegenContext
    from luau_codegen.parse.collect import collect_bindings_root

    ctx = CodegenContext.static()
    bindings = resolve_test_bindings_dir()
    if bindings:
        root = collect_bindings_root(bindings)
        ctx = ctx.with_value_specs(collect_value_struct_specs(root))
    if DELEGATE_FIXTURE_DIR.is_dir():
        raw = collect_delegate_specs(DELEGATE_FIXTURE_DIR, ctx)
        ctx = ctx.with_catalogs(
            value_types=ctx.value_types,
            delegates=build_delegate_catalog(raw, ctx),
        )
    return ctx


DELEGATE_SPECS = fixture_codegen_context().delegates.specs


def all_platforms(value: str = "0x1") -> dict[str, str]:
    return {
        "win": value,
        "m1": value,
        "ios": value,
        "android32": value,
        "android64": value,
    }


def types_text(files: dict[str, str]) -> str:
    from luau_codegen.emit.luau_types import TYPES_FILE

    return files[TYPES_FILE]
