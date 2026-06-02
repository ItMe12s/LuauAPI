from __future__ import annotations

from dataclasses import dataclass
from typing import TYPE_CHECKING, Dict, Iterator, List, Optional, Union

from luau_codegen.parse.broma import Arg, Class, Function, Method
from luau_codegen.convert.type_map import TypeInfo, classify_arg

if TYPE_CHECKING:
    from luau_codegen.model.codegen_context import CodegenContext

CallableWithArgs = Union[Method, Function]


def is_ccobject_ptr(info: TypeInfo) -> bool:
    return info.kind == "object" and info.class_name == "CCObject"


@dataclass(frozen=True)
class LuaMethodArg:
    arg: Arg
    info: TypeInfo
    arg_index: int = 0
    sel_pair: bool = False
    target_arg: Optional[Arg] = None
    target_info: Optional[TypeInfo] = None
    implicit_self_target: bool = False
    handler_first: bool = True
    orphan: bool = False
    out_only: bool = False


def iter_lua_method_args(
    m: CallableWithArgs,
    arg_infos: List[TypeInfo],
    *,
    ret_kind: str,
    is_instance: bool = False,
) -> Iterator[LuaMethodArg]:
    i = 0
    while i < len(m.args):
        arg = m.args[i]
        info = arg_infos[i]
        if ret_kind == "void" and info.is_out:
            yield LuaMethodArg(arg=arg, info=info, arg_index=i, out_only=True)
            i += 1
            continue
        if (
            i + 1 < len(m.args)
            and is_ccobject_ptr(info)
            and arg_infos[i + 1].kind == "sel"
        ):
            yield LuaMethodArg(
                arg=m.args[i + 1],
                info=arg_infos[i + 1],
                arg_index=i + 1,
                sel_pair=True,
                target_arg=arg,
                target_info=info,
                handler_first=True,
            )
            i += 2
            continue
        if (
            i + 1 < len(m.args)
            and info.kind == "sel"
            and is_ccobject_ptr(arg_infos[i + 1])
        ):
            yield LuaMethodArg(
                arg=arg,
                info=info,
                arg_index=i,
                sel_pair=True,
                target_arg=m.args[i + 1],
                target_info=arg_infos[i + 1],
                handler_first=False,
            )
            i += 2
            continue
        if info.kind == "sel" and is_instance:
            yield LuaMethodArg(
                arg=arg,
                info=info,
                arg_index=i,
                sel_pair=True,
                implicit_self_target=True,
                orphan=True,
            )
            i += 1
            continue
        if info.kind == "sel":
            yield LuaMethodArg(
                arg=arg,
                info=info,
                arg_index=i,
                sel_pair=True,
                orphan=True,
            )
            i += 1
            continue
        yield LuaMethodArg(arg=arg, info=info, arg_index=i)
        i += 1


def count_lua_method_args(
    m: CallableWithArgs,
    objects: Dict[str, Class],
    ret_kind: str,
    *,
    owner_class: str = "",
    ctx: CodegenContext | None = None,
) -> int:
    arg_infos = []
    for arg in m.args:
        info = classify_arg(arg.type, objects, owner_class=owner_class, ctx=ctx)
        if info is None:
            return len(m.args)
        arg_infos.append(info)
    is_instance = isinstance(m, Method) and not m.is_static
    return sum(
        1
        for lua_arg in iter_lua_method_args(
            m, arg_infos, ret_kind=ret_kind, is_instance=is_instance
        )
        if not lua_arg.out_only
    )
