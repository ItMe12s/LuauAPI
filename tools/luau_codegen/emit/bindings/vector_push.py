from __future__ import annotations

from luau_codegen.convert.marshalling import push_return, push_value
from luau_codegen.convert.type_map import TypeInfo


def emit_vector_out_push(info: TypeInfo, var: str) -> list[str]:
    if info.kind == "vector_view":
        return push_value(info, var, False, vector_owned=True)
    return push_value(info, var, False)


def emit_owned_vector_return_push(ret: TypeInfo, expr: str) -> list[str]:
    return push_return(ret, expr, False, vector_owned=True)
