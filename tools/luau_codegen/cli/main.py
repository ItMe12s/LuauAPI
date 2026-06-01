from __future__ import annotations

import argparse
import os
from pathlib import Path
import sys
from typing import List

from luau_codegen.util.platforms import VALID_PLATFORMS
from luau_codegen.policy.intersection import intersection_platforms
from luau_codegen.emit import bindings as emit_bindings
from luau_codegen.emit import luau_types as emit_types
from luau_codegen.emit import parity
from luau_codegen.emit import plan as emit_plan
from luau_codegen.emit.plan import ambiguous_overloads
from luau_codegen.parse.collect import collect_bindings_root
from luau_codegen.cli.io import (
    _cleanup_orphans,
    _cleanup_type_orphans,
    _write_if_changed,
)
from luau_codegen.emit.metadata import emit_report, emit_schema


def log_info(message: str) -> None:
    print(f"[luauapi] {message}")


def log_error(message: str) -> None:
    print(f"[luauapi] {message}", file=sys.stderr)


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
        if args.fail_on_ambiguous_overload and ambiguous_overloads(plan):
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
        if args.fail_on_ambiguous_overload and ambiguous_overloads(plan):
            log_error("ambiguous overloads found")
            return 6
        for rel in emit_plan.plan_outputs(root, args.platform, plan=plan):
            print(f"src/{rel}")
        return 0

    if args.list_type_outputs:
        plan = emit_plan.collect_plan(
            root, args.platform, plans_by_platform=plans_by_platform
        )
        if args.fail_on_ambiguous_overload and ambiguous_overloads(plan):
            log_error("ambiguous overloads found")
            return 6
        type_files = emit_types.emit(root, args.platform, plan=plan)
        for name in sorted(type_files):
            print(name)
        return 0

    if not args.out or not args.types_out:
        log_error("--out and --types-out are required")
        return 2

    plan = emit_plan.collect_plan(
        root, args.platform, plans_by_platform=plans_by_platform
    )
    ambiguous = ambiguous_overloads(plan)
    if args.fail_on_ambiguous_overload and ambiguous:
        for cls, method, reason in ambiguous[:200]:
            log_error(f"ambiguous overload: {cls}.{method}: {reason}")
        if len(ambiguous) > 200:
            log_error(f"... {len(ambiguous) - 200} more ambiguous overloads")
        return 6
    written_paths: list[str] = []
    current_files: set[str] = set()

    try:
        binding_files, skipped = emit_bindings.emit(root, args.platform, plan=plan)
        for rel, content in binding_files.items():
            rel_path = os.path.join("src", rel).replace("\\", "/")
            current_files.add(rel_path)
            cxx_path = os.path.join(args.out, rel_path)
            _write_if_changed(cxx_path, content)
            written_paths.append(cxx_path)

        _cleanup_orphans(args.out, current_files)

        schema_path = os.path.join(args.out, "schema.json")
        report_path = os.path.join(args.out, "report.md")
        type_files = emit_types.emit(root, args.platform, plan=plan)
        for filename, content in type_files.items():
            _write_if_changed(os.path.join(args.types_out, filename), content)
        _cleanup_type_orphans(args.types_out, type_files)
        emit_schema(root, schema_path, plan)
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
        emit_report(
            root,
            report_path,
            written_paths,
            ", ".join(types_paths),
            skipped,
            args.platform,
            hook_count,
            field_count,
            plan,
            plans_by_platform,
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
