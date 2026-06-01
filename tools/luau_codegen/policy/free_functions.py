from __future__ import annotations

from dataclasses import dataclass, field

from luau_codegen.convert.type_map import classify_arg, classify_return
from luau_codegen.parse.broma import Class, Function


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


def free_function_supported(fn: Function, objects: dict[str, Class]) -> bool:
    if classify_return(fn.ret, objects) is None:
        return False
    for arg in fn.args:
        info = classify_arg(arg.type, objects)
        if info is None:
            return False
        if info.is_out:
            return False
    return True
