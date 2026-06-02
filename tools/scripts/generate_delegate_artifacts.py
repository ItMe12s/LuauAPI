"""Legacy entry point, prefer: python -m luau_codegen --emit-delegates."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT / "tools") not in sys.path:
    sys.path.insert(0, str(ROOT / "tools"))

from luau_codegen.emit.delegates import (
    default_specs_path,
    emit_delegate_artifacts,
    fallback_bindings_dir,
)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate delegate specs and C++ trampolines"
    )
    parser.add_argument(
        "--bindings",
        help="Path to Geode bindings/<version>/ directory",
    )
    parser.add_argument(
        "--out",
        default=str(ROOT / "build" / "luauapi-gen"),
        help="Output directory for LuaDelegates.gen.* (default: build/luauapi-gen)",
    )
    parser.add_argument(
        "--delegate-specs-out",
        default=str(default_specs_path()),
        help="Output path for delegate_specs.py",
    )
    args = parser.parse_args()
    bindings = args.bindings
    if bindings is None:
        bindings = str(fallback_bindings_dir())
    specs = emit_delegate_artifacts(
        bindings,
        specs_out=args.delegate_specs_out,
        gen_out=args.out,
    )
    print(f"delegates: {len(specs)} -> specs + gen hpp/cpp")


if __name__ == "__main__":
    main()
