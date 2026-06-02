from __future__ import annotations

import re
from pathlib import Path

BRO_DIR = (
    Path(__file__).resolve().parents[2] / "build/_deps/bindings-src/bindings/2.2081"
)


def main() -> None:
    delegate_types: set[str] = set()
    for bro in BRO_DIR.glob("*.bro"):
        text = bro.read_text(encoding="utf-8", errors="replace")
        for m in re.finditer(r"([\w:]+(?:Delegate|Protocol))\*", text):
            delegate_types.add(m.group(1))

    classes: dict[str, list[tuple[str, str, str]]] = {}
    for bro in BRO_DIR.glob("*.bro"):
        text = bro.read_text(encoding="utf-8", errors="replace")
        for m in re.finditer(r"class\s+([\w:]+)\s*\{([^}]*)\}", text, re.DOTALL):
            name = m.group(1)
            if not (name.endswith("Delegate") or name.endswith("Protocol")):
                continue
            body = m.group(2)
            methods = []
            for vm in re.finditer(
                r"virtual\s+([\w:<>,\s*&]+?)\s+(\w+)\s*\(([^)]*)\)", body
            ):
                ret, mn, args = vm.group(1).strip(), vm.group(2), vm.group(3).strip()
                methods.append((mn, ret, args))
            if methods:
                classes[name] = methods

    for t in sorted(delegate_types):
        short = t.split("::")[-1]
        keys = [t, short, f"cocos2d::{short}"]
        found = next((k for k in keys if k in classes), None)
        nmethods = len(classes[found]) if found else 0
        print(f"{t}\t{found or '?'}\t{nmethods}")


if __name__ == "__main__":
    main()
