from __future__ import annotations

from dataclasses import dataclass
from types import MappingProxyType
from collections.abc import Mapping


@dataclass(frozen=True)
class DelegateMethodSpec:
    name: str
    ret_lua: str
    args_lua: tuple[str, ...]


@dataclass(frozen=True)
class DelegateSpec:
    cxx_type: str
    lua_name: str
    cpp_class: str
    create_fn: str
    methods: tuple[DelegateMethodSpec, ...]


@dataclass(frozen=True)
class DelegateCatalog:
    specs: Mapping[str, DelegateSpec]
    cxx_types: frozenset[str]

    @classmethod
    def empty(cls) -> DelegateCatalog:
        return cls(MappingProxyType({}), frozenset())

    @classmethod
    def from_specs(cls, specs: Mapping[str, DelegateSpec]) -> DelegateCatalog:
        owned = MappingProxyType(dict(specs))
        return cls(owned, frozenset(owned))

    def lookup(self, cxx_ptr_type: str) -> DelegateSpec | None:
        n = cxx_ptr_type.strip().removesuffix("*").strip()
        return self.specs.get(n) or self.specs.get(n.split("::")[-1])
