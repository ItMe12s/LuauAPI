from __future__ import annotations

from dataclasses import dataclass, field
from typing import AbstractSet, Mapping, Sequence, Set, Tuple

from luau_codegen.model.geode_enums import EnumInfo


@dataclass(frozen=True)
class CodegenContext:
    geode_enum_names: frozenset[str] = frozenset()
    geode_enum_cxx: Mapping[str, str] = field(default_factory=dict)
    geode_enum_members: Mapping[str, Tuple[Tuple[str, int], ...]] = field(default_factory=dict)
    gd_enum_members: Mapping[str, Tuple[Tuple[str, int], ...]] = field(default_factory=dict)
    cocos_enum_members: Mapping[str, Tuple[Tuple[str, int], ...]] = field(default_factory=dict)

    @classmethod
    def static(cls) -> CodegenContext:
        return cls()

    @classmethod
    def with_geode_enums(
        cls,
        enums: Mapping[str, EnumInfo],
        skip: AbstractSet[str] = frozenset(),
        cocos_enum_members: Mapping[str, Sequence[tuple[str, int]]] | None = None,
    ) -> CodegenContext:
        from luau_codegen.convert.type_map import GD_ENUM_TYPES

        geode_cxx: dict[str, str] = {}
        geode_members: dict[str, tuple[tuple[str, int], ...]] = {}
        gd_members: dict[str, tuple[tuple[str, int], ...]] = {}
        for name, info in enums.items():
            member_tuple = (
                tuple((member.name, member.value) for member in info.members)
                if info.members
                else ()
            )
            if name in skip:
                if name in GD_ENUM_TYPES and member_tuple:
                    gd_members[name] = member_tuple
                continue
            geode_cxx[name] = info.cxx_name
            if member_tuple:
                geode_members[name] = member_tuple
        return cls(
            geode_enum_names=frozenset(geode_cxx.keys()),
            geode_enum_cxx=geode_cxx,
            geode_enum_members=geode_members,
            gd_enum_members=gd_members,
            cocos_enum_members={
                name: tuple((member, int(value)) for member, value in members)
                for name, members in (cocos_enum_members or {}).items()
            },
        )

    def enum_cxx_names(self) -> dict[str, str]:
        from luau_codegen.convert.type_map import STATIC_ENUM_CXX_NAMES

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
        from luau_codegen.convert.type_map import COCOS_ENUM_TYPES, GD_ENUM_TYPES

        if namespace == "geode.cocos2d":
            return set(COCOS_ENUM_TYPES)
        if namespace == "geode":
            return set(self.geode_enum_names)
        return set(GD_ENUM_TYPES)
