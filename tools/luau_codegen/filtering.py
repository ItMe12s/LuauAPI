from __future__ import annotations

from collections import defaultdict
from typing import Dict, List

from broma_parser import Class, Method
from denylist import INACCESSIBLE_CLASSES, INACCESSIBLE_METHODS
from model import short_name, status_for
from type_map import classify_arg, classify_return

STRICT_DIRECT_PLATFORMS: set[str] = set()


def platform_value(m: Method, target_platform: str) -> str:
    value = m.platforms.get(target_platform, "")
    if value:
        return value
    if target_platform == "mac":
        return m.platforms.get("imac", "") or m.platforms.get("m1", "")
    if target_platform == "imac":
        return m.platforms.get("mac", "")
    if target_platform == "m1":
        return m.platforms.get("mac", "")
    if target_platform == "android":
        return m.platforms.get("android64", "") or m.platforms.get("android32", "")
    return ""


def _is_link_platform(cls: Class, target_platform: str) -> bool:
    aliases = {
        "mac": {"mac", "imac", "m1"},
        "imac": {"mac", "imac"},
        "m1": {"mac", "m1"},
        "android": {"android", "android32", "android64"},
        "android32": {"android", "android32"},
        "android64": {"android", "android64"},
    }.get(target_platform, {target_platform})
    for attr in cls.attributes:
        if attr.startswith("link(") and attr.endswith(")"):
            platforms = [p.strip() for p in attr[5:-1].split(",")]
            return any(platform in aliases for platform in platforms)
    return False


def direct_callable(cls: Class, m: Method, target_platform: str) -> bool:
    if _is_link_platform(cls, target_platform):
        return True

    value = platform_value(m, target_platform)
    token = value.split()[0] if value else ""
    if target_platform in STRICT_DIRECT_PLATFORMS:
        return token == "link"
    return bool(value)


def method_key(m: Method) -> str:
    return f"{m.name}({','.join(a.type for a in m.args)})"


def supported(
    cls: Class, m: Method, objects: Dict[str, Class], target_platform: str
) -> tuple[bool, str]:
    if cls.name in INACCESSIBLE_CLASSES:
        return False, "inaccessible-class"
    if (cls.name, m.name) in INACCESSIBLE_METHODS:
        return False, "inaccessible"
    if m.is_ctor:
        return False, "constructor"
    if m.is_dtor:
        return False, "destructor"
    if m.is_callback:
        return False, "callback"
    if m.name.startswith("operator"):
        return False, "operator"
    if status_for(m.platforms) == "Missing":
        return False, "missing-address"
    if not direct_callable(cls, m, target_platform):
        return False, f"not-callable:{target_platform}"
    if classify_return(m.ret, objects) is None:
        return False, f"unsupported-return:{m.ret}"
    for arg in m.args:
        if classify_arg(arg.type, objects) is None:
            return False, f"unsupported-arg:{arg.type}"
    return True, ""


def call_label(cls: Class, m: Method) -> str:
    sep = "." if m.is_static else ":"
    return f"{cls.name}{sep}{m.name}"


def returns_owned(m: Method) -> bool:
    if not m.is_static:
        return False
    name = m.name.lower()
    if name.startswith("create"):
        return True
    if name in ("copy", "clone"):
        return True
    return False


def group_supported(
    cls: Class,
    objects: Dict[str, Class],
    target_platform: str = "win",
) -> tuple[dict[str, list[Method]], list[tuple[Method, str]]]:
    skipped: list[tuple[Method, str]] = []
    by_name: dict[str, list[Method]] = defaultdict(list)
    seen: set[str] = set()
    for m in cls.methods:
        key = method_key(m)
        if key in seen:
            skipped.append((m, "duplicate-signature"))
            continue
        seen.add(key)
        ok, reason = supported(cls, m, objects, target_platform)
        if not ok:
            skipped.append((m, reason))
            continue
        by_name[m.name].append(m)

    for name, methods in list(by_name.items()):
        by_arity: dict[int, list[Method]] = defaultdict(list)
        for m in methods:
            by_arity[len(m.args)].append(m)
        kept: list[Method] = []
        for arity, overloads in by_arity.items():
            if len(overloads) == 1:
                kept.extend(overloads)
            else:
                for m in overloads:
                    skipped.append((m, f"ambiguous-overload-arity:{arity}"))
        if kept:
            by_name[name] = kept
        else:
            del by_name[name]
    return by_name, skipped


def linkless_class_names(
    classes: List[Class],
    objects: Dict[str, Class],
    supported_by_class: dict[str, dict[str, list[Method]]],
    skipped_by_class: dict[str, list[tuple[Method, str]]],
    target_platform: str,
) -> set[str]:
    referenced: set[str] = set()
    for grouped in supported_by_class.values():
        for methods in grouped.values():
            for m in methods:
                ret = classify_return(m.ret, objects)
                if ret and ret.kind == "object":
                    referenced.add(ret.class_name)
                for arg in m.args:
                    info = classify_arg(arg.type, objects)
                    if info and info.kind == "object":
                        referenced.add(info.class_name)

    out: set[str] = set()
    no_call = f"not-callable:{target_platform}"
    for cls in classes:
        if supported_by_class.get(cls.name):
            continue
        reasons = [reason for _, reason in skipped_by_class.get(cls.name, [])]
        if target_platform in STRICT_DIRECT_PLATFORMS and no_call in reasons:
            out.add(cls.name)
            continue
        if cls.name in referenced:
            continue
        if any(reason in ("inaccessible-class", no_call) for reason in reasons):
            out.add(cls.name)

    by_short = {cls.name: cls for cls in classes}
    keep_bases: set[str] = set()
    for cls in classes:
        if cls.name in out:
            continue
        for base in cls.bases:
            base_cls = by_short.get(short_name(base))
            if base_cls:
                keep_bases.add(base_cls.name)
    return out - keep_bases


def _skipped_object_ref(
    m: Method, objects: Dict[str, Class], skipped_classes: set[str]
) -> str:
    ret = classify_return(m.ret, objects)
    if ret and ret.kind == "object" and ret.class_name in skipped_classes:
        return ret.class_name
    for arg in m.args:
        info = classify_arg(arg.type, objects)
        if info and info.kind == "object" and info.class_name in skipped_classes:
            return info.class_name
    return ""


def prune_skipped_class_refs(
    supported_by_class: dict[str, dict[str, list[Method]]],
    skipped_by_class: dict[str, list[tuple[Method, str]]],
    objects: Dict[str, Class],
    skipped_classes: set[str],
    target_platform: str,
) -> list[tuple[str, str, str]]:
    pruned: list[tuple[str, str, str]] = []
    if not skipped_classes:
        return pruned
    for cls_name, grouped in supported_by_class.items():
        for name, methods in list(grouped.items()):
            kept: list[Method] = []
            for m in methods:
                skipped_ref = _skipped_object_ref(m, objects, skipped_classes)
                if skipped_ref:
                    reason = f"not-callable-type:{target_platform}:{skipped_ref}"
                    skipped_by_class[cls_name].append((m, reason))
                    pruned.append((cls_name, m.name, reason))
                else:
                    kept.append(m)
            if kept:
                grouped[name] = kept
            else:
                del grouped[name]
    return pruned
