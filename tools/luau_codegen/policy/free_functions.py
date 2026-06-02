from __future__ import annotations

from collections import defaultdict
from dataclasses import dataclass, field
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from luau_codegen.model.codegen_context import CodegenContext

from luau_codegen.convert.sel_args import is_ccobject_ptr
from luau_codegen.policy.containers import (
    _CONTAINER_KINDS,
    container_supported_as_arg,
    container_supported_as_return,
)
from luau_codegen.convert.type_map import classify_arg, classify_return
from luau_codegen.model.denylist import INACCESSIBLE_CLASSES
from luau_codegen.parse.broma import Class, Function

FreeFunctionSkip = tuple[str, str, str, str]


@dataclass(frozen=True)
class FreeFnOverride:
    namespace: str
    name: str
    allowed_arities_by_platform: dict[str, frozenset[int]] = field(default_factory=dict)
    excluded_platforms: frozenset[str] = frozenset()


_OVERRIDES: tuple[FreeFnOverride, ...] = (
    FreeFnOverride(
        namespace="geode::utils::game",
        name="restart",
        allowed_arities_by_platform={
            "android": frozenset({1}),
            "android32": frozenset({1}),
            "android64": frozenset({1}),
            "ios": frozenset({1}),
            "mac": frozenset({1}),
            "imac": frozenset({1}),
            "m1": frozenset({1}),
        },
    ),
    FreeFnOverride(
        namespace="geode::utils",
        name="getEnvironmentVariable",
        excluded_platforms=frozenset({"ios"}),
    ),
)

_BY_KEY: dict[tuple[str, str], FreeFnOverride] = {
    (o.namespace, o.name): o for o in _OVERRIDES
}


def free_function_skip_reason(fn: Function, target_platform: str) -> str | None:
    rule = _BY_KEY.get((fn.namespace, fn.name))
    if rule is None:
        return None
    if target_platform in rule.excluded_platforms:
        return f"free-function-excluded:{target_platform}"
    arities = rule.allowed_arities_by_platform.get(target_platform)
    if arities is not None and len(fn.args) not in arities:
        return f"free-function-override-arity:{target_platform}"
    return None


def free_function_allowed(fn: Function, target_platform: str) -> bool:
    return free_function_skip_reason(fn, target_platform) is None


def free_function_key(fn: Function) -> str:
    args = ",".join(arg.type for arg in fn.args)
    name = f"{fn.namespace}::{fn.name}" if fn.namespace else fn.name
    return f"{name}({args})"


def free_function_unsupported_reason(
    fn: Function, objects: dict[str, Class], ctx: CodegenContext | None = None
) -> str | None:
    ret = classify_return(fn.ret, objects, ctx=ctx)
    if ret is None:
        return f"free-function-unsupported-return:{fn.ret}"
    if ret.kind in _CONTAINER_KINDS and not container_supported_as_return(ret):
        return f"free-function-unsupported-return:{fn.ret}"
    ret_kind = ret.kind
    for i, arg in enumerate(fn.args):
        info = classify_arg(arg.type, objects, ctx=ctx)
        if info is None:
            return f"free-function-unsupported-arg:{arg.type}"
        if info.kind in _CONTAINER_KINDS and not container_supported_as_arg(
            info, ret_kind
        ):
            return f"free-function-unsupported-arg:{arg.type}"
        if info.is_out and info.kind not in _CONTAINER_KINDS:
            return f"free-function-out-arg:{arg.type}"
        if info.kind == "sel":
            continue
    return None


def free_function_supported(
    fn: Function, objects: dict[str, Class], ctx: CodegenContext | None = None
) -> bool:
    return free_function_unsupported_reason(fn, objects, ctx=ctx) is None


def free_function_skipped_object_ref(
    fn: Function,
    objects: dict[str, Class],
    skipped_classes: set[str],
    ctx: CodegenContext | None = None,
) -> str:
    blocked = skipped_classes | INACCESSIBLE_CLASSES
    ret = classify_return(fn.ret, objects, ctx=ctx)
    if ret and ret.kind == "object" and ret.class_name in blocked:
        return ret.class_name
    for arg in fn.args:
        info = classify_arg(arg.type, objects, ctx=ctx)
        if info and info.kind == "object" and info.class_name in blocked:
            return info.class_name
    return ""


def _skip_entry(fn: Function, reason: str) -> FreeFunctionSkip:
    return (free_function_key(fn), fn.lua_path, fn.name, reason)


def group_supported_free_functions(
    functions: list[Function],
    objects: dict[str, Class],
    target_platform: str = "win",
    ctx: CodegenContext | None = None,
) -> tuple[list[Function], list[FreeFunctionSkip]]:
    skipped: list[FreeFunctionSkip] = []
    by_name: dict[tuple[str, str], list[Function]] = defaultdict(list)

    for fn in functions:
        reason = free_function_unsupported_reason(fn, objects, ctx=ctx)
        if reason:
            skipped.append(_skip_entry(fn, reason))
            continue
        reason = free_function_skip_reason(fn, target_platform)
        if reason:
            skipped.append(_skip_entry(fn, reason))
            continue
        by_name[(fn.namespace, fn.name)].append(fn)

    kept_all: list[Function] = []
    for fns in by_name.values():
        seen_arity: set[int] = set()
        for fn in fns:
            arity = len(fn.args)
            if arity in seen_arity:
                skipped.append(
                    _skip_entry(fn, f"free-function-ambiguous-arity:{arity}")
                )
                continue
            seen_arity.add(arity)
            kept_all.append(fn)
    return kept_all, skipped
