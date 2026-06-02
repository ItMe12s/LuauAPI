from __future__ import annotations

from luau_codegen.convert.type_map import TypeInfo


def delegate_supported_as_arg(info: TypeInfo) -> bool:
    return info.kind == "delegate"


def delegate_supported_as_return(info: TypeInfo) -> bool:
    return info.kind == "delegate"
