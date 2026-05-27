from __future__ import annotations

import copy
from collections import defaultdict
from dataclasses import dataclass, field
from typing import Dict, List, Set

from broma_parser import Class, Method, Root
from filtering import group_supported, linkless_class_names, prune_skipped_class_refs
from hooks import hookable
from intersection import (
    INTERSECTION_PLATFORMS,
    IntersectionResult,
    IntersectionStats,
    collect_intersection,
    method_key,
)
from model import build_class_lookup, codegen_object_map, object_classes, resolve_base


@dataclass
class EmitPlan:
    classes: List[Class]
    objects: Dict[str, Class]
    skipped: List[tuple[str, str, str]]
    skipped_by_class: dict[str, list[tuple[Method, str]]]
    skipped_classes: Set[str]
    supported_by_class: dict[str, dict[str, list[Method]]]
    hook_targets_by_class: dict[str, List[tuple[Class, Method]]]
    depths: dict[str, int]
    emitted_classes: List[Class] = field(default_factory=list)
    intersection_stats: IntersectionStats = field(default_factory=IntersectionStats)


def _binding_filename(cls_name: str) -> str:
    return f"bindings_{cls_name}.cpp"


def _inheritance_depth(
    cls: Class, lookup: Dict[str, Class], skipped_classes: Set[str]
) -> int:
    if cls.name == "CCObject":
        return 0
    values: List[int] = []
    for base in cls.bases:
        base_cls = resolve_base(lookup, base)
        if base_cls and base_cls.name not in skipped_classes:
            values.append(_inheritance_depth(base_cls, lookup, skipped_classes) + 1)
    return max(values) if values else 1


def _emitted_classes(plan: EmitPlan) -> List[Class]:
    out: List[Class] = []
    for cls in plan.classes:
        if cls.name in plan.skipped_classes:
            continue
        grouped = plan.supported_by_class[cls.name]
        if grouped or plan.hook_targets_by_class[cls.name]:
            out.append(cls)
    return out


def collect_platform_plan(root: Root, target_platform: str = "win") -> EmitPlan:
    classes = object_classes(root)
    objects = codegen_object_map(root)
    lookup = build_class_lookup(classes)
    skipped: list[tuple[str, str, str]] = []
    supported_by_class: dict[str, dict[str, list[Method]]] = {}
    skipped_by_class: dict[str, list[tuple[Method, str]]] = {}

    for cls in classes:
        grouped, cls_skipped = group_supported(cls, objects, target_platform)
        supported_by_class[cls.name] = grouped
        skipped_by_class[cls.name] = cls_skipped
        for method, reason in cls_skipped:
            skipped.append((cls.name, method.name, reason))

    skipped_classes = linkless_class_names(
        classes, objects, supported_by_class, skipped_by_class, target_platform
    )
    skipped.extend(
        prune_skipped_class_refs(
            supported_by_class,
            skipped_by_class,
            objects,
            skipped_classes,
            target_platform,
        )
    )
    depths = {
        cls.name: _inheritance_depth(cls, lookup, skipped_classes) for cls in classes
    }

    hook_targets_by_class: dict[str, List[tuple[Class, Method]]] = defaultdict(list)
    for cls in classes:
        if cls.name in skipped_classes:
            continue
        grouped = supported_by_class[cls.name]
        for methods in grouped.values():
            for method in methods:
                if hookable(cls, method, objects, target_platform):
                    hook_targets_by_class[cls.name].append((cls, method))

    plan = EmitPlan(
        classes=classes,
        objects=objects,
        skipped=skipped,
        skipped_by_class=skipped_by_class,
        skipped_classes=skipped_classes,
        supported_by_class=supported_by_class,
        hook_targets_by_class=hook_targets_by_class,
        depths=depths,
    )
    plan.emitted_classes = _emitted_classes(plan)
    return plan


def _intersection_reason(missing: tuple[str, ...]) -> str:
    return "intersection-missing-platform:" + ",".join(missing)


def _append_skip(plan: EmitPlan, cls: Class, method: Method, reason: str) -> None:
    plan.skipped.append((cls.name, method.name, reason))
    plan.skipped_by_class.setdefault(cls.name, []).append((method, reason))


def _apply_intersection(
    plan: EmitPlan,
    target_platform: str,
    result: IntersectionResult,
) -> EmitPlan:
    stats = IntersectionStats(
        enabled=True,
        platforms=result.platforms,
        common_supported_methods=len(result.common_supported_method_keys),
        common_hook_targets=len(result.common_hook_method_keys),
    )
    plan.intersection_stats = stats

    for cls in plan.classes:
        grouped = plan.supported_by_class[cls.name]
        for name, methods in list(grouped.items()):
            kept: List[Method] = []
            for method in methods:
                key = method_key(cls, method)
                if key in result.common_supported_method_keys:
                    kept.append(method)
                    continue
                missing = result.missing_supported_platforms_by_key.get(key, ())
                if not missing:
                    missing = tuple(
                        platform
                        for platform in result.platforms
                        if platform != target_platform
                    )
                reason = _intersection_reason(missing)
                _append_skip(plan, cls, method, reason)
                stats.removed_methods.append((cls.name, method.name, reason))
            if kept:
                grouped[name] = kept
            else:
                del grouped[name]

        kept_hooks: List[tuple[Class, Method]] = []
        for hook_cls, method in plan.hook_targets_by_class.get(cls.name, []):
            key = method_key(hook_cls, method)
            if key in result.common_hook_method_keys:
                kept_hooks.append((hook_cls, method))
                continue
            missing = result.missing_hook_platforms_by_key.get(key, ())
            reason = _intersection_reason(missing)
            stats.removed_hooks.append((hook_cls.name, method.name, reason))
        plan.hook_targets_by_class[cls.name] = kept_hooks

    changed = True
    while changed:
        changed = False
        outputless = {
            cls.name
            for cls in plan.classes
            if not plan.supported_by_class[cls.name]
            and not plan.hook_targets_by_class[cls.name]
        }
        new_skipped = outputless - plan.skipped_classes
        if new_skipped:
            plan.skipped_classes.update(new_skipped)
            changed = True

        pruned = prune_skipped_class_refs(
            plan.supported_by_class,
            plan.skipped_by_class,
            plan.objects,
            plan.skipped_classes,
            target_platform,
        )
        if pruned:
            plan.skipped.extend(pruned)
            changed = True

        for cls_name in list(plan.hook_targets_by_class):
            if (
                cls_name in plan.skipped_classes
                and plan.hook_targets_by_class[cls_name]
            ):
                plan.hook_targets_by_class[cls_name] = []
                changed = True

    lookup = build_class_lookup(plan.classes)
    plan.depths = {
        cls.name: _inheritance_depth(cls, lookup, plan.skipped_classes)
        for cls in plan.classes
    }
    plan.emitted_classes = _emitted_classes(plan)
    return plan


def collect_plan(
    root: Root,
    target_platform: str = "win",
    plans_by_platform: dict[str, EmitPlan] | None = None,
) -> EmitPlan:
    raw_plans = plans_by_platform or {}
    missing = [
        platform for platform in INTERSECTION_PLATFORMS if platform not in raw_plans
    ]
    if missing:
        raw_plans = dict(raw_plans)
        for platform in missing:
            raw_plans[platform] = collect_platform_plan(root, platform)

    if target_platform in raw_plans:
        plan = (
            copy.deepcopy(raw_plans[target_platform])
            if plans_by_platform
            else raw_plans[target_platform]
        )
    else:
        plan = collect_platform_plan(root, target_platform)

    result = collect_intersection(raw_plans)
    return _apply_intersection(plan, target_platform, result)


def plan_outputs(
    root: Root,
    target_platform: str = "win",
    plan: EmitPlan | None = None,
    plans_by_platform: dict[str, EmitPlan] | None = None,
) -> List[str]:
    if plan is None:
        plan = collect_plan(root, target_platform, plans_by_platform=plans_by_platform)
    outputs = ["bindings_internal.hpp", "bindings_common.cpp"]
    outputs.extend(_binding_filename(cls.name) for cls in plan.emitted_classes)
    return outputs


def hook_target_count(
    root: Root,
    target_platform: str = "win",
    plan: EmitPlan | None = None,
    plans_by_platform: dict[str, EmitPlan] | None = None,
) -> int:
    if plan is None:
        plan = collect_plan(root, target_platform, plans_by_platform=plans_by_platform)
    return sum(len(targets) for targets in plan.hook_targets_by_class.values())
