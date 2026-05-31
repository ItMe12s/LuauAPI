from __future__ import annotations

import argparse
import glob
import json
import os
from pathlib import Path
import sys
import warnings
from typing import List

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)

import broma_parser
import emit_luau_bindings
import emit_luau_types
import parity
import plan as emit_plan
from fields import field_key
from intersection import intersection_platforms
from model import BRO_FILES, object_classes, status_for

VALID_PLATFORMS = {
    "win",
    "android",
    "android32",
    "android64",
    "ios",
    "mac",
    "imac",
    "m1",
}


def log_info(message: str) -> None:
    print(f"[luauapi] {message}")


def log_error(message: str) -> None:
    print(f"[luauapi] {message}", file=sys.stderr)


def collect_bindings_root(
    bindings_dir: str, geode_sdk_path: str | None = None
) -> broma_parser.Root:
    root = broma_parser.Root()
    for name in BRO_FILES:
        path = os.path.join(bindings_dir, name)
        if not os.path.exists(path):
            warnings.warn(f"[luauapi] missing Broma file: {path}")
            continue
        parsed = broma_parser.parse_file(path)
        root.classes.extend(parsed.classes)
    if geode_sdk_path:
        from geode_sdk_scanner import (
            scan_geode_sdk,
            scan_geode_enums,
            scan_geode_functions,
        )
        from type_map import register_geode_enums

        root.classes.extend(scan_geode_sdk(geode_sdk_path))
        register_geode_enums(scan_geode_enums(geode_sdk_path))
        root.functions.extend(scan_geode_functions(geode_sdk_path))
    from filtering import method_key

    seen: dict[str, broma_parser.Class] = {}
    for cls in root.classes:
        if cls.qualified_name in seen:
            existing = seen[cls.qualified_name]
            for attr in cls.attributes:
                if attr not in existing.attributes:
                    existing.attributes.append(attr)
            if cls.methods:
                if not existing.methods:
                    existing.methods = list(cls.methods)
                else:
                    known = {method_key(existing, m) for m in existing.methods}
                    for method in cls.methods:
                        key = method_key(cls, method)
                        if key not in known:
                            existing.methods.append(method)
                            known.add(key)
        else:
            seen[cls.qualified_name] = cls
    root.classes = list(seen.values())
    root.classes.sort(key=lambda c: (c.namespace, c.name))
    return root


def _collect_extra_dluau() -> str:
    extra_dir = os.path.join(HERE, "extra_bindings")
    if not os.path.isdir(extra_dir):
        return ""
    parts: list[str] = []
    for name in sorted(os.listdir(extra_dir)):
        if name.endswith(".dluau"):
            with open(os.path.join(extra_dir, name), "r", encoding="utf-8") as f:
                parts.append(f.read().strip())
    if not parts:
        return ""
    body = "\n\n".join(parts)
    return (
        "\n\n-- Custom definitions from tools/luau_codegen/extra_bindings/\n"
        + body
        + "\n"
    )


def _write_if_changed(path: str, content: str) -> None:
    try:
        with open(path, "r", encoding="utf-8") as f:
            if f.read() == content:
                return
    except FileNotFoundError:
        pass
    parent = os.path.dirname(path)
    if parent:
        os.makedirs(parent, exist_ok=True)
    tmp_path = f"{path}.tmp.{os.getpid()}"
    try:
        with open(tmp_path, "w", encoding="utf-8", newline="\n") as f:
            f.write(content)
        os.replace(tmp_path, path)
    except OSError:
        try:
            if os.path.exists(tmp_path):
                os.remove(tmp_path)
        finally:
            raise


def _ambiguous_overloads(plan: emit_plan.EmitPlan) -> list[tuple[str, str, str]]:
    return [
        (cls, method, reason)
        for cls, method, reason in plan.skipped
        if reason.startswith("ambiguous-overload-arity:")
    ]


def _emit_schema(root: broma_parser.Root, path: str, plan: emit_plan.EmitPlan) -> None:
    bound_fields = {
        field_key(cls, field)
        for targets in plan.field_targets_by_class.values()
        for cls, field in targets
    }
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
                    {
                        "name": f.name,
                        "type": f.type,
                        "count": f.count,
                        "line": f.line,
                        "bound": field_key(cls, f) in bound_fields,
                    }
                    for f in cls.fields
                ],
            }
        )
    ambiguous = [
        {"class": cls, "method": method, "reason": reason}
        for cls, method, reason in _ambiguous_overloads(plan)
    ]
    _write_if_changed(
        path,
        json.dumps(
            {
                "classes": classes,
                "ambiguousOverloads": ambiguous,
            },
            indent=2,
        )
        + "\n",
    )


def _emit_report(
    root: broma_parser.Root,
    path: str,
    cxx_paths: list[str],
    types_path: str,
    skipped: list[tuple[str, str, str]],
    target_platform: str,
    hook_target_count: int,
    field_target_count: int,
    plan: emit_plan.EmitPlan | None = None,
) -> None:
    obj = object_classes(root)
    total_methods = sum(len(c.methods) for c in root.classes)
    emitted_methods = total_methods - len(skipped)
    reasons: dict[str, int] = {}
    for _, _, reason in skipped:
        reasons[reason] = reasons.get(reason, 0) + 1
    lines = [
        "# LuauAPI codegen report\n\n",
        f"- Target platform: **{target_platform}**\n",
        f"- Classes parsed: **{len(root.classes)}**\n",
        f"- CCObject classes emitted: **{len(obj)}**\n",
        f"- Methods parsed: **{total_methods}**\n",
        f"- Methods emitted: **{emitted_methods}**\n",
        f"- Methods skipped: **{len(skipped)}**\n",
        f"- Hook targets emitted: **{hook_target_count}**\n",
        f"- Fields bound: **{field_target_count}**\n",
        f"- Generated binding files: **{len(cxx_paths)}**\n",
    ]
    if plan and plan.intersection_stats.enabled:
        stats = plan.intersection_stats
        lines.extend(
            [
                "- Intersection mode: **enabled**\n",
                f"- Intersection platforms: **{', '.join(stats.platforms)}**\n",
                f"- Common binding methods: **{stats.common_supported_methods}**\n",
                f"- Common hook targets: **{stats.common_hook_targets}**\n",
                f"- Common fields: **{stats.common_fields}**\n",
                f"- Methods removed by intersection: **{len(stats.removed_methods)}**\n",
                f"- Hooks removed by intersection: **{len(stats.removed_hooks)}**\n",
            ]
        )
    for cxx_path in cxx_paths:
        lines.append(f"  - `{cxx_path}`\n")
    lines.append(f"- Generated Luau types: `{types_path}`\n\n")
    lines.append("## Skip reasons\n\n")
    for reason, count in sorted(reasons.items(), key=lambda i: (-i[1], i[0])):
        lines.append(f"- {reason}: {count}\n")
    ambiguous = _ambiguous_overloads(plan) if plan else []
    lines.append("\n## Ambiguous overloads\n\n")
    if ambiguous:
        for cls, method, reason in ambiguous[:2000]:
            lines.append(f"- {cls}.{method}: {reason}\n")
        if len(ambiguous) > 2000:
            lines.append(f"- ... {len(ambiguous) - 2000} more\n")
    else:
        lines.append("- none\n")
    lines.append("\n## Operational notes\n\n")
    lines.append(
        "- generated Luau types are written to repo `types/` for LSP use and remain gitignored\n"
    )
    lines.append(
        "- geode_bindings is pinned via LUAUAPI_BINDINGS_GIT_TAG in CMake, bump mod.json GD version and the pin together\n"
    )
    lines.append(
        "- generated binding .cpp files are listed at CMake configure via --list-outputs, reconfigure when new bindings_<Class>.cpp are emitted\n"
    )
    lines.append(
        "- userdata tag budget assert remains generated-class based, runtime distinct-type tightening is deferred\n"
    )
    lines.append(
        "- removed hook callback slots compact on later registry operations, eager compaction is deferred\n"
    )
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
    if sys.version_info < (3, 11):
        log_error("Python 3.11 or newer is required")
        return 2

    parser = argparse.ArgumentParser()
    parser.add_argument("--bindings", required=True)
    parser.add_argument("--out")
    parser.add_argument("--types-out")
    parser.add_argument("--platform", default="win")
    parser.add_argument("--geode-sdk", default=None)
    parser.add_argument(
        "--list-outputs",
        action="store_true",
        help="Print generated binding paths relative to --out/src, one per line",
    )
    parser.add_argument(
        "--list-type-outputs",
        action="store_true",
        help="Print generated type file names, one per line",
    )
    parser.add_argument(
        "--parity-report-out",
        help="Write runtime-safe cross-platform parity report and exit",
    )
    parser.add_argument(
        "--fail-on-ambiguous-overload",
        action="store_true",
        help="Fail codegen when same-arity overloads require first-declared selection",
    )
    args = parser.parse_args(argv)

    if args.platform not in VALID_PLATFORMS:
        valid = ", ".join(sorted(VALID_PLATFORMS))
        log_error(f"unknown platform '{args.platform}'. Expected one of: {valid}")
        return 2

    args.bindings = str(Path(args.bindings).resolve())
    if args.out:
        args.out = str(Path(args.out).resolve())
    if args.types_out:
        args.types_out = str(Path(args.types_out).resolve())
    if args.geode_sdk:
        args.geode_sdk = str(Path(args.geode_sdk).resolve())
    if args.parity_report_out:
        args.parity_report_out = str(Path(args.parity_report_out).resolve())

    if not os.path.isdir(args.bindings):
        log_error(f"bindings dir missing: {args.bindings}")
        return 2

    try:
        root = collect_bindings_root(args.bindings, geode_sdk_path=args.geode_sdk)
    except OSError as exc:
        log_error(f"I/O failed while collecting bindings: {exc}")
        return 4

    if not root.classes:
        log_error(f"no Broma classes parsed from {args.bindings}")
        return 3

    plan_platforms = tuple(
        dict.fromkeys(intersection_platforms(args.platform) + (args.platform,))
    )
    plans_by_platform = {
        platform: emit_plan.collect_platform_plan(root, platform)
        for platform in plan_platforms
    }

    if args.parity_report_out:
        plan = emit_plan.collect_plan(
            root, args.platform, plans_by_platform=plans_by_platform
        )
        if args.fail_on_ambiguous_overload and _ambiguous_overloads(plan):
            log_error("ambiguous overloads found")
            return 6
        try:
            parity_data = parity.collect_parity(
                root,
                plans_by_platform=plans_by_platform,
                target_platform=args.platform,
            )
            _write_if_changed(args.parity_report_out, parity.emit_markdown(parity_data))
        except OSError as exc:
            log_error(f"I/O failed while writing parity report: {exc}")
            return 4
        log_info(f"wrote {args.parity_report_out}")
        return 0

    if args.list_outputs:
        plan = emit_plan.collect_plan(
            root, args.platform, plans_by_platform=plans_by_platform
        )
        if args.fail_on_ambiguous_overload and _ambiguous_overloads(plan):
            log_error("ambiguous overloads found")
            return 6
        for rel in emit_plan.plan_outputs(root, args.platform, plan=plan):
            print(f"src/{rel}")
        return 0

    if args.list_type_outputs:
        plan = emit_plan.collect_plan(
            root, args.platform, plans_by_platform=plans_by_platform
        )
        if args.fail_on_ambiguous_overload and _ambiguous_overloads(plan):
            log_error("ambiguous overloads found")
            return 6
        type_files = emit_luau_types.emit(root, args.platform, plan=plan)
        for name in sorted(type_files):
            print(name)
        return 0

    if not args.out or not args.types_out:
        log_error("--out and --types-out are required")
        return 2

    plan = emit_plan.collect_plan(
        root, args.platform, plans_by_platform=plans_by_platform
    )
    ambiguous = _ambiguous_overloads(plan)
    if args.fail_on_ambiguous_overload and ambiguous:
        for cls, method, reason in ambiguous[:200]:
            log_error(f"ambiguous overload: {cls}.{method}: {reason}")
        if len(ambiguous) > 200:
            log_error(f"... {len(ambiguous) - 200} more ambiguous overloads")
        return 6
    written_paths: list[str] = []
    current_files: set[str] = set()

    try:
        binding_files, skipped = emit_luau_bindings.emit(root, args.platform, plan=plan)
        for rel, content in binding_files.items():
            rel_path = os.path.join("src", rel).replace("\\", "/")
            current_files.add(rel_path)
            cxx_path = os.path.join(args.out, rel_path)
            _write_if_changed(cxx_path, content)
            written_paths.append(cxx_path)

        _cleanup_orphans(args.out, current_files)

        schema_path = os.path.join(args.out, "schema.json")
        report_path = os.path.join(args.out, "report.md")
        type_files = emit_luau_types.emit(root, args.platform, plan=plan)
        extra_dluau = _collect_extra_dluau()
        if extra_dluau and "geode.d.luau" in type_files:
            type_files["geode.d.luau"] += extra_dluau
        for filename, content in type_files.items():
            _write_if_changed(os.path.join(args.types_out, filename), content)
        orphan_globs = ("*.d.luau", "luau-lsp.json")
        for pattern in orphan_globs:
            for orphan in glob.glob(os.path.join(args.types_out, pattern)):
                name = os.path.basename(orphan)
                if name not in type_files:
                    os.remove(orphan)
        _emit_schema(root, schema_path, plan)
        parity_path = os.path.join(args.out, "parity.json")
        parity_data = parity.collect_parity(
            root, plans_by_platform=plans_by_platform, target_platform=args.platform
        )
        _write_if_changed(parity_path, parity.emit_json(parity_data))
    except OSError as exc:
        log_error(f"I/O failed while writing generated files: {exc}")
        return 4
    except Exception as exc:
        log_error(f"codegen failed: {exc}")
        return 5

    types_paths = [os.path.join(args.types_out, f) for f in type_files]
    hook_count = emit_plan.hook_target_count(root, args.platform, plan=plan)
    field_count = emit_plan.field_target_count(root, args.platform, plan=plan)
    try:
        _emit_report(
            root,
            report_path,
            written_paths,
            ", ".join(types_paths),
            skipped,
            args.platform,
            hook_count,
            field_count,
            plan,
        )
    except OSError as exc:
        log_error(f"I/O failed while writing report: {exc}")
        return 4
    log_info(f"parsed {len(root.classes)} classes")
    log_info(f"target platform {args.platform}")
    log_info(f"hook targets {hook_count}")
    log_info(f"fields bound {field_count}")
    for path in written_paths:
        log_info(f"wrote {path}")
    for path in types_paths:
        log_info(f"wrote {path}")
    log_info(f"wrote {parity_path}")
    log_info(f"wrote {report_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
