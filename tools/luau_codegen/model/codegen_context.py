from __future__ import annotations

from dataclasses import dataclass, field, replace
from types import MappingProxyType
from collections.abc import Mapping, Sequence, Set

from luau_codegen.convert.type_primitives import (
    COCOS_ENUM_TYPES,
    GD_ENUM_TYPES,
    STATIC_ENUM_CXX_NAMES,
)
from luau_codegen.model.geode_enums import EnumInfo
from luau_codegen.model.delegate_specs import DelegateCatalog
from luau_codegen.model.value_types import ValueTypeCatalog, ValueTypeSpec


@dataclass(frozen=True)
class CodegenContext:
    geode_enum_names: frozenset[str] = frozenset()
    geode_enum_cxx: Mapping[str, str] = field(default_factory=dict)
    geode_enum_members: Mapping[str, tuple[tuple[str, int], ...]] = field(default_factory=dict)
    gd_enum_members: Mapping[str, tuple[tuple[str, int], ...]] = field(default_factory=dict)
    cocos_enum_members: Mapping[str, tuple[tuple[str, int], ...]] = field(default_factory=dict)
    value_types: ValueTypeCatalog = field(default_factory=ValueTypeCatalog.builtins)
    delegates: DelegateCatalog = field(default_factory=DelegateCatalog.empty)
    _enum_cxx_names: Mapping[str, str] = field(init=False, repr=False, compare=False)
    _enum_types: frozenset[str] = field(init=False, repr=False, compare=False)
    _enum_lua_names: Mapping[str, frozenset[str]] = field(init=False, repr=False, compare=False)

    def __post_init__(self) -> None:
        object.__setattr__(self, "geode_enum_names", frozenset(self.geode_enum_names))
        object.__setattr__(self, "geode_enum_cxx", MappingProxyType(dict(self.geode_enum_cxx)))
        object.__setattr__(
            self,
            "geode_enum_members",
            MappingProxyType({k: tuple(v) for k, v in self.geode_enum_members.items()}),
        )
        object.__setattr__(
            self,
            "gd_enum_members",
            MappingProxyType({k: tuple(v) for k, v in self.gd_enum_members.items()}),
        )
        object.__setattr__(
            self,
            "cocos_enum_members",
            MappingProxyType({k: tuple(v) for k, v in self.cocos_enum_members.items()}),
        )
        names = dict(STATIC_ENUM_CXX_NAMES)
        for name, cxx in self.geode_enum_cxx.items():
            names.setdefault(name, cxx)
        object.__setattr__(self, "_enum_cxx_names", MappingProxyType(names))
        object.__setattr__(self, "_enum_types", frozenset(names))
        object.__setattr__(
            self,
            "_enum_lua_names",
            MappingProxyType(
                {
                    "geode.cocos2d": frozenset(COCOS_ENUM_TYPES),
                    "geode": self.geode_enum_names,
                    "geode.gd": frozenset(GD_ENUM_TYPES),
                }
            ),
        )

    @classmethod
    def static(cls) -> CodegenContext:
        return cls()

    @classmethod
    def with_geode_enums(
        cls,
        enums: Mapping[str, EnumInfo],
        skip: Set[str] = frozenset(),
        cocos_enum_members: Mapping[str, Sequence[tuple[str, int]]] | None = None,
        *,
        value_types: ValueTypeCatalog | None = None,
        delegates: DelegateCatalog | None = None,
    ) -> CodegenContext:
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
            value_types=value_types or ValueTypeCatalog.builtins(),
            delegates=delegates or DelegateCatalog.empty(),
        )

    def with_value_specs(self, specs: Sequence[ValueTypeSpec]) -> CodegenContext:
        return replace(self, value_types=ValueTypeCatalog.from_specs(specs))

    def with_catalogs(
        self,
        *,
        value_types: ValueTypeCatalog,
        delegates: DelegateCatalog,
    ) -> CodegenContext:
        return replace(self, value_types=value_types, delegates=delegates)

    @property
    def enum_types(self) -> frozenset[str]:
        return self._enum_types

    def enum_cxx_type(self, n: str, base: str) -> str:
        names = self._enum_cxx_names
        if n in names:
            return names[n]
        if base in names:
            return names[base]
        return "int"

    def enum_lua_names(self, namespace: str) -> frozenset[str]:
        return self._enum_lua_names.get(namespace, self._enum_lua_names["geode.gd"])
