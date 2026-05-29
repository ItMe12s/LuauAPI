from __future__ import annotations

import json
from collections import Counter
from typing import Any

from broma_parser import Root
from plan import (
    EmitPlan,
    collect_plan,
    collect_platform_plan,
    field_target_count,
    hook_target_count,
    plan_outputs,
)
from hooks import hook_address_expr
from intersection import (
    hook_method_keys,
    intersection_platforms,
    method_key,
    supported_method_keys,
)
from model import object_classes


def _supported_keys(plan: EmitPlan) -> set[str]:
    return supported_method_keys(plan)


def _hookable_keys(plan: EmitPlan) -> set[str]:
    return hook_method_keys(plan)


def _skip_reasons(plan: EmitPlan) -> dict[str, str]:
    out: dict[str, str] = {}
    classes_by_name = {cls.name: cls for cls in plan.classes}
    for cls_name, skipped in plan.skipped_by_class.items():
        cls = classes_by_name.get(cls_name)
        if not cls:
            continue
        for method, reason in skipped:
            out[method_key(cls, method)] = reason

    for cls in plan.classes:
        if cls.name not in plan.skipped_classes:
            continue
        for method in cls.methods:
            key = method_key(cls, method)
            out.setdefault(key, "skipped-class")
    return out


def collect_parity(
    root: Root,
    platforms: tuple[str, ...] | None = None,
    plans_by_platform: dict[str, EmitPlan] | None = None,
    target_platform: str = "win",
) -> dict[str, Any]:
    if platforms is None:
        platforms = intersection_platforms(target_platform)
    plans = dict(plans_by_platform or {})
    for platform in platforms:
        if platform not in plans:
            plans[platform] = collect_platform_plan(root, platform)
    final_plan = collect_plan(root, platforms[0], plans_by_platform=plans)
    total_methods = sum(len(cls.methods) for cls in root.classes)
    object_count = len(object_classes(root))

    summary: dict[str, dict[str, int]] = {}
    supported_by_platform: dict[str, set[str]] = {}
    hookable_by_platform: dict[str, set[str]] = {}
    skip_reasons_by_platform: dict[str, dict[str, str]] = {}

    for platform, plan in plans.items():
        supported = _supported_keys(plan)
        hookable_keys = _hookable_keys(plan)
        skipped = _skip_reasons(plan)
        supported_by_platform[platform] = supported
        hookable_by_platform[platform] = hookable_keys
        skip_reasons_by_platform[platform] = skipped
        summary[platform] = {
            "classesParsed": len(root.classes),
            "ccObjectClasses": object_count,
            "methodsParsed": total_methods,
            "methodsEmitted": total_methods - len(plan.skipped),
            "methodsSkipped": len(plan.skipped),
            "hookTargets": hook_target_count(root, platform, plan=plan),
            "fieldsBound": field_target_count(root, platform, plan=plan),
            "generatedBindingFiles": len(plan_outputs(root, platform, plan=plan)),
            "skippedClasses": len(plan.skipped_classes),
            "emittedClasses": len(plan.emitted_classes),
        }

    methods: dict[str, dict[str, Any]] = {}
    for cls in object_classes(root):
        for method in cls.methods:
            key = method_key(cls, method)
            supported_platforms = [
                platform
                for platform in platforms
                if key in supported_by_platform[platform]
            ]
            hookable_platforms = [
                platform
                for platform in platforms
                if key in hookable_by_platform[platform]
            ]
            skip_reasons = {
                platform: skip_reasons_by_platform[platform][key]
                for platform in platforms
                if key in skip_reasons_by_platform[platform]
            }
            hook_address_missing_platforms = [
                platform
                for platform in supported_platforms
                if not method.is_static
                and not method.is_ctor
                and not method.is_dtor
                and platform not in hookable_platforms
                and not hook_address_expr(cls, method, platform)
            ]
            methods[key] = {
                "class": cls.name,
                "qualifiedClass": cls.qualified_name,
                "name": method.name,
                "signature": ",".join(arg.type for arg in method.args),
                "ret": method.ret,
                "supportedPlatforms": supported_platforms,
                "hookablePlatforms": hookable_platforms,
                "hookAddressMissingPlatforms": hook_address_missing_platforms,
                "skipReasons": skip_reasons,
            }

    hints = _collect_hints(plans, methods, platforms)
    return {
        "platforms": list(platforms),
        "summary": summary,
        "intersection": _intersection_summary(final_plan, total_methods),
        "methods": methods,
        "skippedClasses": {
            platform: sorted(plans[platform].skipped_classes) for platform in platforms
        },
        "hints": hints,
    }


def _intersection_summary(plan: EmitPlan, total_methods: int) -> dict[str, Any]:
    stats = plan.intersection_stats
    return {
        "enabled": stats.enabled,
        "platforms": list(stats.platforms),
        "commonBindingMethods": stats.common_supported_methods,
        "commonHookTargets": stats.common_hook_targets,
        "commonFields": stats.common_fields,
        "methodsEmitted": total_methods - len(plan.skipped),
        "methodsSkipped": len(plan.skipped),
        "hookTargets": sum(
            len(targets) for targets in plan.hook_targets_by_class.values()
        ),
        "generatedBindingFiles": 2 + len(plan.emitted_classes),
        "emittedClasses": len(plan.emitted_classes),
        "skippedClasses": len(plan.skipped_classes),
        "methodsRemoved": len(stats.removed_methods),
        "hooksRemoved": len(stats.removed_hooks),
        "fieldsRemoved": len(stats.removed_fields),
    }


def _collect_hints(
    plans: dict[str, EmitPlan],
    methods: dict[str, dict[str, Any]],
    platforms: tuple[str, ...],
) -> dict[str, Any]:
    supported_elsewhere = {
        key: [
            platform
            for platform in platforms
            if platform != "win" and platform in info["supportedPlatforms"]
        ]
        for key, info in methods.items()
    }
    win_missing = [
        key
        for key, info in methods.items()
        if info["skipReasons"].get("win") in ("missing-address", "not-callable:win")
        and supported_elsewhere[key]
    ]
    ios_pruned = sorted(plans["ios"].skipped_classes) if "ios" in plans else []
    m1_reasons = Counter(
        info["skipReasons"].get("m1")
        for info in methods.values()
        if "android64" in info["supportedPlatforms"]
        and "m1" not in info["supportedPlatforms"]
        and info["skipReasons"].get("m1")
    )
    hook_only_gaps = [
        key for key, info in methods.items() if info["hookAddressMissingPlatforms"]
    ]
    return {
        "winMissingCallableProof": win_missing[:200],
        "winMissingCallableProofCount": len(win_missing),
        "iosSkippedClasses": ios_pruned[:200],
        "iosSkippedClassCount": len(ios_pruned),
        "m1GapReasons": dict(sorted(m1_reasons.items())),
        "hookAddressGaps": hook_only_gaps[:200],
        "hookAddressGapCount": len(hook_only_gaps),
    }


def emit_json(data: dict[str, Any]) -> str:
    return json.dumps(data, indent=2, sort_keys=True) + "\n"


def emit_markdown(data: dict[str, Any]) -> str:
    lines = ["# LuauAPI parity report\n\n"]
    lines.append("## Summary\n\n")
    lines.append(
        "| Platform | Methods emitted | Methods skipped | Hook targets | Fields bound | Binding files | Skipped classes |\n"
    )
    lines.append("| --- | ---: | ---: | ---: | ---: | ---: | ---: |\n")
    for platform in data["platforms"]:
        row = data["summary"][platform]
        lines.append(
            f"| {platform} | {row['methodsEmitted']} | {row['methodsSkipped']} | "
            f"{row['hookTargets']} | {row.get('fieldsBound', 0)} | {row['generatedBindingFiles']} | {row['skippedClasses']} |\n"
        )

    if "intersection" in data:
        final = data["intersection"]
        lines.append("\n## Forced Intersection\n\n")
        lines.append(f"- enabled: {str(final['enabled']).lower()}\n")
        lines.append(f"- platforms: {', '.join(final['platforms'])}\n")
        lines.append(f"- common binding methods: {final['commonBindingMethods']}\n")
        lines.append(f"- common hook targets: {final['commonHookTargets']}\n")
        lines.append(f"- common fields: {final.get('commonFields', 0)}\n")
        lines.append(f"- generated binding files: {final['generatedBindingFiles']}\n")
        lines.append(f"- methods removed: {final['methodsRemoved']}\n")
        lines.append(f"- hooks removed: {final['hooksRemoved']}\n")
        lines.append(f"- fields removed: {final.get('fieldsRemoved', 0)}\n")

    lines.append("\n## Runtime-Safe Hints\n\n")
    hints = data["hints"]
    lines.append(
        f"- win missing callable proof: {hints['winMissingCallableProofCount']} methods. "
        "Add verified win offsets or link(win) metadata only when true.\n"
    )
    lines.append(
        f"- ios skipped classes: {hints['iosSkippedClassCount']} classes. "
        "Strict iOS filtering prunes classes without callable proof.\n"
    )
    lines.append(
        f"- hook address gaps: {hints['hookAddressGapCount']} methods. "
        "Callable binding exists, but hook address proof is absent on at least one supported platform.\n"
    )
    if hints["m1GapReasons"]:
        reason_text = ", ".join(
            f"{reason}: {count}" for reason, count in hints["m1GapReasons"].items()
        )
    else:
        reason_text = "none"
    lines.append(f"- m1 gaps versus android64: {reason_text}.\n")

    lines.append("\n## Samples\n\n")
    _append_sample(
        lines, "win missing callable proof", hints["winMissingCallableProof"]
    )
    _append_sample(lines, "ios skipped classes", hints["iosSkippedClasses"])
    _append_sample(lines, "hook address gaps", hints["hookAddressGaps"])
    return "".join(lines)


def _append_sample(lines: list[str], title: str, values: list[str]) -> None:
    lines.append(f"### {title}\n\n")
    if not values:
        lines.append("- none\n\n")
        return
    for value in values[:25]:
        lines.append(f"- `{value}`\n")
    if len(values) > 25:
        lines.append(f"- ... {len(values) - 25} more\n")
    lines.append("\n")
