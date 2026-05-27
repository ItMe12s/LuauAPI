from __future__ import annotations

import json
from collections import Counter
from typing import Any

from broma_parser import Class, Method, Root
from emit_luau_bindings import EmitPlan, collect_plan, hook_target_count, plan_outputs
from hooks import hook_address_expr
from model import object_classes

PARITY_PLATFORMS = ("win", "m1", "ios", "android32", "android64")


def method_key(cls: Class, method: Method) -> str:
    args = ",".join(arg.type for arg in method.args)
    return f"{cls.qualified_name}.{method.name}({args})"


def _supported_keys(plan: EmitPlan) -> set[str]:
    out: set[str] = set()
    classes_by_name = {cls.name: cls for cls in plan.classes}
    for cls_name, grouped in plan.supported_by_class.items():
        if cls_name in plan.skipped_classes:
            continue
        cls = classes_by_name.get(cls_name)
        if not cls:
            continue
        for methods in grouped.values():
            for method in methods:
                out.add(method_key(cls, method))
    return out


def _hookable_keys(plan: EmitPlan) -> set[str]:
    out: set[str] = set()
    for targets in plan.hook_targets_by_class.values():
        for cls, method in targets:
            out.add(method_key(cls, method))
    return out


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
    root: Root, platforms: tuple[str, ...] = PARITY_PLATFORMS
) -> dict[str, Any]:
    plans = {platform: collect_plan(root, platform) for platform in platforms}
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
        "methods": methods,
        "skippedClasses": {
            platform: sorted(plans[platform].skipped_classes) for platform in platforms
        },
        "hints": hints,
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
        key
        for key, info in methods.items()
        if info["hookAddressMissingPlatforms"]
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
        "| Platform | Methods emitted | Methods skipped | Hook targets | Binding files | Skipped classes |\n"
    )
    lines.append("| --- | ---: | ---: | ---: | ---: | ---: |\n")
    for platform in data["platforms"]:
        row = data["summary"][platform]
        lines.append(
            f"| {platform} | {row['methodsEmitted']} | {row['methodsSkipped']} | "
            f"{row['hookTargets']} | {row['generatedBindingFiles']} | {row['skippedClasses']} |\n"
        )

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
    _append_sample(lines, "win missing callable proof", hints["winMissingCallableProof"])
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
