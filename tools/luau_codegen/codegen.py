from __future__ import annotations

import argparse
import glob
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
    cxx_paths: list[str],
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
        f"- Generated binding files: **{len(cxx_paths)}**\n",
    ]
    for cxx_path in cxx_paths:
        lines.append(f"  - `{cxx_path}`\n")
    lines.append(f"- Generated Luau types: `{types_path}`\n\n")
    lines.append("## Skip reasons\n\n")
    for reason, count in sorted(reasons.items(), key=lambda i: (-i[1], i[0])):
        lines.append(f"- {reason}: {count}\n")
    lines.append("\n## Skipped methods\n\n")
    for cls, method, reason in skipped[:2000]:
        lines.append(f"- {cls}.{method}: {reason}\n")
    if len(skipped) > 2000:
        lines.append(f"- ... {len(skipped) - 2000} more\n")
    _write_if_changed(path, "".join(lines))


def _cleanup_orphans(out_dir: str, current_files: set[str]) -> None:
    src_dir = os.path.join(out_dir, "src")
    legacy = os.path.join(src_dir, "luauapi_generated_bindings.cpp")
    if os.path.exists(legacy):
        os.remove(legacy)

    if not os.path.isdir(src_dir):
        return

    for path in glob.glob(os.path.join(src_dir, "bindings_*.cpp")):
        rel = os.path.relpath(path, out_dir).replace("\\", "/")
        if rel not in current_files:
            os.remove(path)

    legacy_hpp = os.path.join(src_dir, "bindings_internal.hpp")
    if "src/bindings_internal.hpp" not in current_files and os.path.exists(legacy_hpp):
        os.remove(legacy_hpp)


def main(argv: List[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--bindings", required=True)
    parser.add_argument("--out")
    parser.add_argument("--types-out")
    parser.add_argument("--platform", default="win")
    parser.add_argument(
        "--list-outputs",
        action="store_true",
        help="Print generated binding paths relative to --out/src, one per line",
    )
    args = parser.parse_args(argv)

    if not os.path.isdir(args.bindings):
        print(f"[luauapi] bindings dir missing: {args.bindings}", file=sys.stderr)
        return 2

    root = _collect(args.bindings)
    if not root.classes:
        print(
            f"[luauapi] no Broma classes parsed from {args.bindings}", file=sys.stderr
        )
        return 3

    if args.list_outputs:
        for rel in emit_luau_bindings.plan_outputs(root, args.platform):
            print(f"src/{rel}")
        return 0

    if not args.out or not args.types_out:
        print("[luauapi] --out and --types-out are required", file=sys.stderr)
        return 2

    binding_files, skipped = emit_luau_bindings.emit(root, args.platform)
    written_paths: list[str] = []
    current_files: set[str] = set()

    for rel, content in binding_files.items():
        rel_path = os.path.join("src", rel).replace("\\", "/")
        current_files.add(rel_path)
        cxx_path = os.path.join(args.out, rel_path)
        _write_if_changed(cxx_path, content)
        written_paths.append(cxx_path)

    _cleanup_orphans(args.out, current_files)

    schema_path = os.path.join(args.out, "schema.json")
    report_path = os.path.join(args.out, "report.md")
    type_files = emit_luau_types.emit(root, args.platform)
    for filename, content in type_files.items():
        _write_if_changed(os.path.join(args.types_out, filename), content)
    _emit_schema(root, schema_path)
    types_paths = [os.path.join(args.types_out, f) for f in type_files]
    _emit_report(root, report_path, written_paths, ", ".join(types_paths), skipped)
    print(f"[luauapi] parsed {len(root.classes)} classes")
    for path in written_paths:
        print(f"[luauapi] wrote {path}")
    for path in types_paths:
        print(f"[luauapi] wrote {path}")
    print(f"[luauapi] wrote {report_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
