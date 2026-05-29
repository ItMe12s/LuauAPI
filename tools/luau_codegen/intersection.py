from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any

from fields import field_key
from filtering import method_key

INTERSECTION_PLATFORMS = ("win", "m1", "ios", "android32", "android64")


def intersection_platforms(target_platform: str = "win") -> tuple[str, ...]:
    mac = "imac" if target_platform == "imac" else "m1"
    return ("win", mac, "ios", "android32", "android64")


@dataclass
class IntersectionResult:
    platforms: tuple[str, ...]
    common_supported_method_keys: set[str]
    common_hook_method_keys: set[str]
    common_field_keys: set[str]
    missing_supported_platforms_by_key: dict[str, tuple[str, ...]]
    missing_hook_platforms_by_key: dict[str, tuple[str, ...]]
    missing_field_platforms_by_key: dict[str, tuple[str, ...]]


@dataclass
class IntersectionStats:
    enabled: bool = False
    platforms: tuple[str, ...] = INTERSECTION_PLATFORMS
    common_supported_methods: int = 0
    common_hook_targets: int = 0
    common_fields: int = 0
    removed_methods: list[tuple[str, str, str]] = field(default_factory=list)
    removed_hooks: list[tuple[str, str, str]] = field(default_factory=list)
    removed_fields: list[tuple[str, str, str]] = field(default_factory=list)


def supported_method_keys(plan: Any) -> set[str]:
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


def hook_method_keys(plan: Any) -> set[str]:
    out: set[str] = set()
    for targets in plan.hook_targets_by_class.values():
        for cls, method in targets:
            out.add(method_key(cls, method))
    return out


def bound_field_keys(plan: Any) -> set[str]:
    out: set[str] = set()
    for targets in getattr(plan, "field_targets_by_class", {}).values():
        for cls, field in targets:
            out.add(field_key(cls, field))
    return out


def collect_intersection(
    plans_by_platform: dict[str, Any],
    platforms: tuple[str, ...] = INTERSECTION_PLATFORMS,
) -> IntersectionResult:
    supported_by_platform = {
        platform: supported_method_keys(plans_by_platform[platform])
        for platform in platforms
    }
    hooks_by_platform = {
        platform: hook_method_keys(plans_by_platform[platform])
        for platform in platforms
    }
    fields_by_platform = {
        platform: bound_field_keys(plans_by_platform[platform])
        for platform in platforms
    }

    all_supported = set().union(*supported_by_platform.values())
    all_hooks = set().union(*hooks_by_platform.values())
    all_fields = set().union(*fields_by_platform.values())
    common_supported = set.intersection(*supported_by_platform.values())
    common_hooks = set.intersection(*hooks_by_platform.values())
    common_fields = set.intersection(*fields_by_platform.values())

    missing_supported = {
        key: tuple(
            platform
            for platform in platforms
            if key not in supported_by_platform[platform]
        )
        for key in all_supported
        if key not in common_supported
    }
    missing_hooks = {
        key: tuple(
            platform for platform in platforms if key not in hooks_by_platform[platform]
        )
        for key in all_hooks
        if key not in common_hooks
    }
    missing_fields = {
        key: tuple(
            platform
            for platform in platforms
            if key not in fields_by_platform[platform]
        )
        for key in all_fields
        if key not in common_fields
    }

    return IntersectionResult(
        platforms=platforms,
        common_supported_method_keys=common_supported,
        common_hook_method_keys=common_hooks,
        common_field_keys=common_fields,
        missing_supported_platforms_by_key=missing_supported,
        missing_hook_platforms_by_key=missing_hooks,
        missing_field_platforms_by_key=missing_fields,
    )
