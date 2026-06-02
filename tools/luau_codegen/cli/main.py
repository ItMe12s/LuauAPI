from __future__ import annotations

import argparse
import os
from pathlib import Path
import subprocess
import sys
from typing import List

from luau_codegen.util.platforms import VALID_PLATFORMS
from luau_codegen.policy.intersection import intersection_platforms
from luau_codegen.emit import bindings as emit_bindings
from luau_codegen.emit import luau_types as emit_types
from luau_codegen.emit import parity
from luau_codegen.emit import plan as emit_plan
from luau_codegen.emit.plan import ambiguous_overloads
from luau_codegen.emit.delegates import (
    default_specs_path,
    delegate_gen_rel_paths,
    emit_delegate_artifacts,
)
from luau_codegen.parse.collect import collect_bindings_root
from luau_codegen.cli.io import (
    _cleanup_orphans,
    _cleanup_type_orphans,
    _write_if_changed,
)
from luau_codegen.emit.metadata import emit_report, emit_schema
from luau_codegen.emit import audit as emit_audit


def log_info(message: str) -> None:
    print(f"[luauapi] {message}")


def log_error(message: str) -> None:
    print(f"[luauapi] {message}", file=sys.stderr)


def _repo_root() -> Path:
    return Path(__file__).resolve().parents[3]


def _run_delegate_codegen(bindings: str, out: str, specs_out: str) -> int:
    cmd = [
        sys.executable,
        "-m",
        "luau_codegen",
        "--emit-delegates",
        "--bindings",
        bindings,
        "--out",
        out,
        "--delegate-specs-out",
        specs_out,
    ]
    env = os.environ.copy()
    tools_dir = str(_repo_root() / "tools")
    existing = env.get("PYTHONPATH", "")
    env["PYTHONPATH"] = f"{tools_dir}{os.pathsep}{existing}" if existing else tools_dir
    result = subprocess.run(cmd, env=env)
    return result.returncode


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
        "--audit-report-out",
        help="Write skip audit report and exit",
    )
    parser.add_argument(
        "--fail-on-ambiguous-overload",
        action="store_true",
        help="Fail codegen when same-arity overloads require first-declared selection",
    )
    parser.add_argument(
        "--emit-delegates",
        action="store_true",
        help="Generate delegate_specs.py and LuaDelegates.gen.* only",
    )
    parser.add_argument(
        "--delegate-specs-out",
        help="Output path for delegate_specs.py (default: tools/luau_codegen/model/delegate_specs.py)",
    )
    parser.add_argument(
        "--skip-delegate-emit",
        action="store_true",
        help="Skip delegate artifact generation (used when a prior build step already emitted them)",
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
    if args.audit_report_out:
        args.audit_report_out = str(Path(args.audit_report_out).resolve())

    if args.delegate_specs_out:
        args.delegate_specs_out = str(Path(args.delegate_specs_out).resolve())

    if not os.path.isdir(args.bindings):
        log_error(f"bindings dir missing: {args.bindings}")
        return 2

    if args.emit_delegates:
        if not args.out:
            log_error("--out is required with --emit-delegates")
            return 2
        specs_out = args.delegate_specs_out or str(default_specs_path())
        try:
            specs = emit_delegate_artifacts(
                args.bindings,
                specs_out=specs_out,
                gen_out=args.out,
            )
        except OSError as exc:
            log_error(f"I/O failed while writing delegate artifacts: {exc}")
            return 4
        log_info(f"delegates: {len(specs)} specs -> {specs_out}")
        for rel in delegate_gen_rel_paths():
            log_info(f"wrote {os.path.join(args.out, rel)}")
        return 0

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

    if args.audit_report_out:
        plan = emit_plan.collect_plan(
            root, args.platform, plans_by_platform=plans_by_platform
        )
        if args.fail_on_ambiguous_overload and ambiguous_overloads(plan):
            log_error("ambiguous overloads found")
            return 6
        try:
            audit_data = emit_audit.collect_audit(plan, root)
            _write_if_changed(
                args.audit_report_out, emit_audit.emit_markdown(audit_data)
            )
        except OSError as exc:
            log_error(f"I/O failed while writing audit report: {exc}")
            return 4
        log_info(f"wrote {args.audit_report_out}")
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
        for rel in delegate_gen_rel_paths():
            print(rel)
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

    specs_out = args.delegate_specs_out or str(default_specs_path())
    if not args.skip_delegate_emit:
        delegate_status = _run_delegate_codegen(args.bindings, args.out, specs_out)
        if delegate_status != 0:
            return delegate_status

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
    audit_path = os.path.join(args.out, "audit.md")
    try:
        audit_data = emit_audit.collect_audit(plan, root)
        _write_if_changed(audit_path, emit_audit.emit_markdown(audit_data))
    except OSError as exc:
        log_error(f"I/O failed while writing audit report: {exc}")
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
    log_info(f"wrote {audit_path}")
    return 0
