from __future__ import annotations

import dataclasses
from collections import defaultdict
from typing import Dict, List

from luau_codegen.parse.broma import Class, Method
from luau_codegen.model.denylist import (
    BINDABLE_CONSTRUCTORS,
    INACCESSIBLE_CLASSES,
    INACCESSIBLE_METHODS,
    PREFERRED_OVERLOADS,
)
from luau_codegen.model.domain import build_class_lookup, resolve_base, status_for
from luau_codegen.policy.link_attrs import is_link_platform, platform_aliases
from luau_codegen.convert.type_map import (
    classify_arg,
    classify_return,
    method_input_arg_count,
    normalize_type,
)
from luau_codegen.convert.sel_args import is_ccobject_ptr
from luau_codegen.policy.containers import (
    vector_view_supported_as_arg,
    vector_view_supported_as_return,
)
from luau_codegen.policy.delegates import (
    delegate_supported_as_arg,
    delegate_supported_as_return,
)

STRICT_DIRECT_PLATFORMS: set[str] = {"ios"}


def _platform_aliases(target_platform: str) -> set[str]:
    return platform_aliases(target_platform)


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


def _method_missing_platforms(m: Method) -> set[str]:
    out: set[str] = set()
    for attr in m.attributes:
        if attr.startswith("missing(") and attr.endswith(")"):
            out.update(p.strip() for p in attr[8:-1].split(","))
    return out


def _is_missing_on_platform(m: Method, target_platform: str) -> bool:
    missing = _method_missing_platforms(m)
    if not missing:
        return False
    return bool(missing & _platform_aliases(target_platform))


def direct_callable(cls: Class, m: Method, target_platform: str) -> bool:
    if is_link_platform(cls, target_platform):
        return True
    if m.platforms.get("inline") == "inline":
        return True

    value = platform_value(m, target_platform)
    token = value.split()[0] if value else ""
    if target_platform in STRICT_DIRECT_PLATFORMS:
        return token == "link" or token.startswith("0x") or token == "inline"
    return bool(value)


def method_key(cls: Class, m: Method) -> str:
    args = ",".join(a.type for a in m.args)
    return f"{cls.qualified_name}.{m.name}({args})"


def _ctor_signature(m: Method) -> tuple[str, ...]:
    return tuple(normalize_type(a.type) for a in m.args)


def _allowed_ctor(cls: Class, m: Method) -> bool:
    allowed = BINDABLE_CONSTRUCTORS.get(cls.name)
    return bool(allowed and _ctor_signature(m) in allowed)


def as_new_factory(cls: Class, m: Method) -> Method:
    return dataclasses.replace(
        m,
        name="new",
        ret=f"{cls.name}*",
        is_static=True,
        is_ctor=False,
        is_bound_ctor=True,
    )


def supported(
    cls: Class, m: Method, objects: Dict[str, Class], target_platform: str
) -> tuple[bool, str]:
    if cls.name in INACCESSIBLE_CLASSES:
        return False, "inaccessible-class"
    if (cls.name, m.name) in INACCESSIBLE_METHODS:
        return False, "inaccessible"
    if m.is_ctor and not _allowed_ctor(cls, m):
        return False, "constructor"
    if m.is_dtor:
        return False, "destructor"
    if m.is_callback:
        return False, "callback"
    if m.access != "public":
        return False, "inaccessible"
    if m.name.startswith("operator"):
        return False, "operator"
    if _is_missing_on_platform(m, target_platform):
        return False, "missing-platform"
    if status_for(m.platforms) == "Missing":
        if not is_link_platform(cls, target_platform):
            return False, "missing-address"
        if m.name.startswith("_"):
            return False, "inaccessible"
    elif not direct_callable(cls, m, target_platform):
        return False, f"not-callable:{target_platform}"
    ret = classify_return(m.ret, objects)
    if ret is None:
        return False, f"unsupported-return:{m.ret}"
    if ret.kind == "vector_view" and not vector_view_supported_as_return(ret):
        return False, f"unsupported-return:{m.ret}"
    if ret.kind == "delegate" and not delegate_supported_as_return(ret):
        return False, f"unsupported-return:{m.ret}"
    for i, arg in enumerate(m.args):
        info = classify_arg(arg.type, objects, owner_class=cls.name)
        if info is None:
            return False, f"unsupported-arg:{arg.type}"
        if info.kind == "vector_view" and not vector_view_supported_as_arg(
            info, ret.kind
        ):
            return False, f"unsupported-arg:{arg.type}"
        if info.kind == "delegate" and not delegate_supported_as_arg(info):
            return False, f"unsupported-arg:{arg.type}"
        if info.kind == "sel":
            continue
    return True, ""


def call_label(cls: Class, m: Method) -> str:
    sep = "." if m.is_static else ":"
    return f"{cls.name}{sep}{m.name}"


def returns_owned(m: Method) -> bool:
    if "luau_owned" in m.attributes:
        return True
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
        key = method_key(cls, m)
        if key in seen:
            skipped.append((m, "duplicate-signature"))
            continue
        seen.add(key)
        ok, reason = supported(cls, m, objects, target_platform)
        if not ok:
            skipped.append((m, reason))
            continue
        if m.is_ctor:
            m = as_new_factory(cls, m)
        by_name[m.name].append(m)

    for name, methods in list(by_name.items()):
        by_arity: dict[int, list[Method]] = defaultdict(list)
        for m in methods:
            by_arity[method_input_arg_count(m, objects, owner_class=cls.name)].append(m)
        kept: list[Method] = []
        preferred = PREFERRED_OVERLOADS.get((cls.name, name))
        for arity, overloads in by_arity.items():
            if len(overloads) == 1:
                kept.extend(overloads)
                continue
            chosen: Method | None = None
            if preferred:
                for m in overloads:
                    sig = tuple(normalize_type(a.type) for a in m.args)
                    if sig in preferred:
                        chosen = m
                        break
            if chosen is not None:
                kept.append(chosen)
                for m in overloads:
                    if m is not chosen:
                        skipped.append((m, f"overload-superseded:{arity}"))
            else:
                kept.append(overloads[0])
                for m in overloads[1:]:
                    skipped.append((m, f"ambiguous-overload-arity:{arity}"))
        if kept:
            by_name[name] = kept
        else:
            del by_name[name]
    return by_name, skipped


def _class_has_platform_support(cls: Class, target_platform: str) -> bool:
    if is_link_platform(cls, target_platform):
        return True
    for m in cls.methods:
        if m.is_static:
            continue
        value = platform_value(m, target_platform)
        if not value:
            continue
        token = value.split()[0]
        if token.startswith("0x") or token == "link" or token == "inline":
            return True
    return False


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
            if (
                target_platform in STRICT_DIRECT_PLATFORMS
                and not _class_has_platform_support(cls, target_platform)
            ):
                out.add(cls.name)
            continue
        reasons = [reason for _, reason in skipped_by_class.get(cls.name, [])]
        if target_platform in STRICT_DIRECT_PLATFORMS and no_call in reasons:
            out.add(cls.name)
            continue
        if cls.name in referenced:
            if (
                target_platform not in STRICT_DIRECT_PLATFORMS
                or _class_has_platform_support(cls, target_platform)
            ):
                continue
            out.add(cls.name)
            continue
        if any(reason in ("inaccessible-class", no_call) for reason in reasons):
            out.add(cls.name)

    lookup = build_class_lookup(classes)
    keep_bases: set[str] = set()
    for cls in classes:
        if cls.name in out:
            continue
        for base in cls.bases:
            base_cls = resolve_base(lookup, base)
            if base_cls:
                keep_bases.add(base_cls.name)
    return out - keep_bases


def _is_skipped_object_type(class_name: str, skipped_classes: set[str]) -> bool:
    return class_name in skipped_classes or class_name in INACCESSIBLE_CLASSES


def _skipped_object_ref(
    m: Method, objects: Dict[str, Class], skipped_classes: set[str]
) -> str:
    ret = classify_return(m.ret, objects)
    if (
        ret
        and ret.kind == "object"
        and _is_skipped_object_type(ret.class_name, skipped_classes)
    ):
        return ret.class_name
    for arg in m.args:
        info = classify_arg(arg.type, objects)
        if (
            info
            and info.kind == "object"
            and _is_skipped_object_type(info.class_name, skipped_classes)
        ):
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
