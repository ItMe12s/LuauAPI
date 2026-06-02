from __future__ import annotations

from dataclasses import dataclass, field
from typing import AbstractSet, Dict, Mapping, Set

from luau_codegen.convert.type_map import (
    COCOS_ENUM_TYPES,
    GD_ENUM_TYPES,
    STATIC_ENUM_CXX_NAMES,
)


@dataclass(frozen=True)
class CodegenContext:
    geode_enum_names: frozenset[str] = frozenset()
    geode_enum_cxx: Mapping[str, str] = field(default_factory=dict)

    @classmethod
    def static(cls) -> CodegenContext:
        return cls()

    @classmethod
    def with_geode_enums(
        cls,
        names_to_cxx: Dict[str, str],
        skip: AbstractSet[str] = frozenset(),
    ) -> CodegenContext:
        geode: dict[str, str] = {}
        for name, cxx in names_to_cxx.items():
            if name in skip:
                continue
            geode[name] = cxx
        return cls(
            geode_enum_names=frozenset(geode.keys()),
            geode_enum_cxx=geode,
        )

    def enum_cxx_names(self) -> dict[str, str]:
        merged = dict(STATIC_ENUM_CXX_NAMES)
        for name, cxx in self.geode_enum_cxx.items():
            merged.setdefault(name, cxx)
        return merged

    @property
    def enum_types(self) -> frozenset[str]:
        return frozenset(self.enum_cxx_names().keys())

    def enum_cxx_type(self, n: str, base: str) -> str:
        names = self.enum_cxx_names()
        if n in names:
            return names[n]
        if base in names:
            return names[base]
        return "int"

    def enum_lua_names(self, namespace: str) -> Set[str]:
        if namespace == "geode.cocos2d":
            return set(COCOS_ENUM_TYPES)
        if namespace == "geode":
            return set(self.geode_enum_names)
        return set(GD_ENUM_TYPES)
