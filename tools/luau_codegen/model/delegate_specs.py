from __future__ import annotations

from dataclasses import dataclass
from typing import Dict, FrozenSet, Optional, Tuple


@dataclass(frozen=True)
class DelegateMethodSpec:
    name: str
    ret_lua: str
    args_lua: Tuple[str, ...]


@dataclass(frozen=True)
class DelegateSpec:
    cxx_type: str
    lua_name: str
    cpp_class: str
    create_fn: str
    methods: Tuple[DelegateMethodSpec, ...]


DELEGATE_SPECS: Dict[str, DelegateSpec] = {}
DELEGATE_CXX_TYPES: FrozenSet[str] = frozenset()


def lookup_delegate(cxx_ptr_type: str) -> Optional[DelegateSpec]:
    n = cxx_ptr_type.strip().removesuffix("*").strip()
    return DELEGATE_SPECS.get(n) or DELEGATE_SPECS.get(n.split("::")[-1])
