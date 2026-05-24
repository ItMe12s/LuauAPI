from __future__ import annotations

from collections import defaultdict
from typing import Dict, List

from broma_parser import Class, Method
from denylist import INACCESSIBLE_CLASSES, INACCESSIBLE_METHODS
from model import short_name, status_for
from type_map import classify_arg, classify_return


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
    if not platform_value(m, target_platform) and not cls.source.endswith(
        "Cocos2d.bro"
    ):
        return False, f"not-linkable:{target_platform}"
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
    no_link = f"not-linkable:{target_platform}"
    for cls in classes:
        if supported_by_class.get(cls.name):
            continue
        reasons = [reason for _, reason in skipped_by_class.get(cls.name, [])]
        if cls.name in referenced:
            continue
        if any(reason in ("inaccessible-class", no_link) for reason in reasons):
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
