from __future__ import annotations

import argparse
import json
import os
import sys
from typing import List

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)

import broma_parser
import emit_luau_bindings
import emit_luau_types
from model import BRO_FILES, object_classes, status_for


def _collect(bindings_dir: str) -> broma_parser.Root:
    root = broma_parser.Root()
    for name in BRO_FILES:
        path = os.path.join(bindings_dir, name)
        if not os.path.exists(path):
            continue
        parsed = broma_parser.parse_file(path)
        root.classes.extend(parsed.classes)
    root.classes.sort(key=lambda c: (c.namespace, c.name))
    return root


def _write_if_changed(path: str, content: str) -> None:
    try:
        with open(path, "r", encoding="utf-8") as f:
            if f.read() == content:
                return
    except FileNotFoundError:
        pass
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(content)


def _emit_schema(root: broma_parser.Root, path: str) -> None:
    classes = []
    for cls in root.classes:
        classes.append(
            {
                "name": cls.name,
                "qualifiedName": cls.qualified_name,
                "bases": cls.bases,
                "attributes": cls.attributes,
                "source": os.path.basename(cls.source),
                "line": cls.line,
                "methods": [
                    {
                        "name": m.name,
                        "ret": m.ret,
                        "args": [{"type": a.type, "name": a.name} for a in m.args],
                        "static": m.is_static,
                        "virtual": m.is_virtual,
                        "const": m.is_const,
                        "ctor": m.is_ctor,
                        "dtor": m.is_dtor,
                        "platforms": m.platforms,
                        "status": status_for(m.platforms),
                        "line": m.line,
                    }
                    for m in cls.methods
                ],
                "fields": [
                    {"name": f.name, "type": f.type, "count": f.count, "line": f.line}
                    for f in cls.fields
                ],
            }
        )
    _write_if_changed(path, json.dumps({"classes": classes}, indent=2) + "\n")


def _emit_report(
    root: broma_parser.Root,
    path: str,
    cxx_path: str,
    types_path: str,
    skipped: list[tuple[str, str, str]],
) -> None:
    obj = object_classes(root)
    total_methods = sum(len(c.methods) for c in root.classes)
    emitted_methods = total_methods - len(skipped)
    reasons: dict[str, int] = {}
    for _, _, reason in skipped:
        reasons[reason] = reasons.get(reason, 0) + 1
    lines = [
        "# LuauAPI codegen report\n\n",
        f"- Classes parsed: **{len(root.classes)}**\n",
        f"- CCObject classes emitted: **{len(obj)}**\n",
        f"- Methods parsed: **{total_methods}**\n",
        f"- Methods emitted: **{emitted_methods}**\n",
        f"- Methods skipped: **{len(skipped)}**\n",
        f"- Generated C++: `{cxx_path}`\n",
        f"- Generated Luau types: `{types_path}`\n\n",
        "## Skip reasons\n\n",
    ]
    for reason, count in sorted(reasons.items(), key=lambda i: (-i[1], i[0])):
        lines.append(f"- {reason}: {count}\n")
    lines.append("\n## Skipped methods\n\n")
    for cls, method, reason in skipped[:2000]:
        lines.append(f"- {cls}.{method}: {reason}\n")
    if len(skipped) > 2000:
        lines.append(f"- ... {len(skipped) - 2000} more\n")
    _write_if_changed(path, "".join(lines))


def main(argv: List[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--bindings", required=True)
    parser.add_argument("--out", required=True)
    parser.add_argument("--types-out", required=True)
    parser.add_argument("--platform", default="win")
    args = parser.parse_args(argv)

    if not os.path.isdir(args.bindings):
        print(f"[luauapi] bindings dir missing: {args.bindings}", file=sys.stderr)
        return 2

    root = _collect(args.bindings)
    if not root.classes:
        print(f"[luauapi] no Broma classes parsed from {args.bindings}", file=sys.stderr)
        return 3

    cxx, skipped = emit_luau_bindings.emit(root, args.platform)
    cxx_path = os.path.join(args.out, "src", "luauapi_generated_bindings.cpp")
    schema_path = os.path.join(args.out, "schema.json")
    report_path = os.path.join(args.out, "report.md")
    _write_if_changed(cxx_path, cxx)
    _write_if_changed(args.types_out, emit_luau_types.emit(root, args.platform))
    _emit_schema(root, schema_path)
    _emit_report(root, report_path, cxx_path, args.types_out, skipped)
    print(f"[luauapi] parsed {len(root.classes)} classes")
    print(f"[luauapi] wrote {cxx_path}")
    print(f"[luauapi] wrote {args.types_out}")
    print(f"[luauapi] wrote {report_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
