from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT / "tools"), str(ROOT / "tools" / "scripts")]

from generate_delegate_artifacts import (
    BRO_DIR,
    collect,
    parse_broma,
)

FIXTURE_DIR = ROOT / "tests/luau_codegen/fixtures/delegate_bindings"


def scan_delegate_ptrs(bindings_dir: Path) -> set[str]:
    ptrs: set[str] = set()
    for bro in bindings_dir.glob("*.bro"):
        text = bro.read_text(encoding="utf-8", errors="replace")
        for match in re.finditer(r"([\w:]+(?:Delegate|Protocol))\*", text):
            ptrs.add(match.group(1))
    return ptrs


def emit_fixture_bro(bindings_dir: Path) -> str:
    broma = parse_broma()
    ptrs = scan_delegate_ptrs(bindings_dir)
    short_ptrs = {ptr.split("::")[-1] for ptr in ptrs}

    lines = ["// Offline CI fixture, minimal delegate bindings snapshot"]
    for name, methods in sorted(broma.items()):
        if not (name.endswith("Delegate") or name.endswith("Protocol")):
            continue
        short = name.split("::")[-1]
        if name not in ptrs and short not in short_ptrs:
            continue
        lines.append(f"class {short} {{")
        for method in methods:
            args = ", ".join(
                f"{arg_type} {arg_name}" for arg_type, arg_name in method.args
            )
            lines.append(f"    virtual {method.ret} {method.name}({args}) {{}}")
        lines.append("};")
        lines.append("")

    lines.append("class DelegatePtrHost {")
    for ptr in sorted(ptrs):
        short = ptr.split("::")[-1]
        prefix = ptr.rsplit("::", 1)[0] if "::" in ptr else ""
        cxx_type = f"{prefix}::{short}*" if prefix else f"{short}*"
        lines.append(f"    void set_{short}({cxx_type} delegate) {{}}")
    lines.append("};")
    return "\n".join(lines) + "\n"


def main() -> None:
    if not BRO_DIR.is_dir() or not any(BRO_DIR.glob("*.bro")):
        raise SystemExit(f"bindings dir missing or empty: {BRO_DIR}")

    FIXTURE_DIR.mkdir(parents=True, exist_ok=True)
    fixture_path = FIXTURE_DIR / "Delegates.bro"
    fixture_path.write_text(emit_fixture_bro(BRO_DIR), encoding="utf-8")

    specs = collect(FIXTURE_DIR)
    print(f"wrote {fixture_path}")
    print(f"fixture delegate specs: {len(specs)}")


if __name__ == "__main__":
    main()
