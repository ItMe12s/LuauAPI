from __future__ import annotations

from dataclasses import dataclass
from luau_codegen.parse.broma import Function


@dataclass(frozen=True)
class FreeFnOverride:
    namespace: str
    name: str
    allowed_arities_by_platform: dict[str, frozenset[int]]


_OVERRIDES: tuple[FreeFnOverride, ...] = (
    FreeFnOverride(
        namespace="geode::utils::game",
        name="restart",
        allowed_arities_by_platform={
            "android": frozenset({1}),
            "android32": frozenset({1}),
            "android64": frozenset({1}),
        },
    ),
)

_BY_KEY: dict[tuple[str, str], FreeFnOverride] = {
    (o.namespace, o.name): o for o in _OVERRIDES
}


def free_function_allowed(fn: Function, target_platform: str) -> bool:
    rule = _BY_KEY.get((fn.namespace, fn.name))
    if rule is None:
        return True
    arities = rule.allowed_arities_by_platform.get(target_platform)
    if arities is None:
        return True
    return len(fn.args) in arities
