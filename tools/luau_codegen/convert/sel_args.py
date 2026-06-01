from __future__ import annotations

from dataclasses import dataclass
from typing import Dict, Iterator, List, Optional, Tuple, Union

from luau_codegen.parse.broma import Arg, Class, Function, Method
from luau_codegen.convert.type_map import TypeInfo, classify_arg

CallableWithArgs = Union[Method, Function]


def is_ccobject_ptr(info: TypeInfo) -> bool:
    return info.kind == "object" and info.class_name == "CCObject"


@dataclass(frozen=True)
class LuaMethodArg:
    arg: Arg
    info: TypeInfo
    sel_pair: bool = False
    target_arg: Optional[Arg] = None
    target_info: Optional[TypeInfo] = None


def iter_lua_method_args(
    m: CallableWithArgs,
    arg_infos: List[TypeInfo],
    *,
    ret_kind: str,
) -> Iterator[LuaMethodArg]:
    i = 0
    while i < len(m.args):
        arg = m.args[i]
        info = arg_infos[i]
        if ret_kind == "void" and info.is_out:
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
                sel_pair=True,
                target_arg=arg,
                target_info=info,
            )
            i += 2
            continue
        yield LuaMethodArg(arg=arg, info=info)
        i += 1


def count_lua_method_args(
    m: CallableWithArgs, objects: Dict[str, Class], ret_kind: str
) -> int:
    arg_infos = []
    for arg in m.args:
        info = classify_arg(arg.type, objects)
        if info is None:
            return len(m.args)
        arg_infos.append(info)
    return sum(1 for _ in iter_lua_method_args(m, arg_infos, ret_kind=ret_kind))


def collapse_sel_menu_pairs(
    args: List[Arg],
    arg_infos: List[TypeInfo],
) -> Tuple[List[Arg], List[TypeInfo]]:
    lua_args: List[Arg] = []
    lua_infos: List[TypeInfo] = []
    i = 0
    while i < len(args):
        info = arg_infos[i]
        if (
            i + 1 < len(args)
            and is_ccobject_ptr(info)
            and arg_infos[i + 1].kind == "sel"
        ):
            lua_args.append(args[i + 1])
            lua_infos.append(arg_infos[i + 1])
            i += 2
            continue
        lua_args.append(args[i])
        lua_infos.append(info)
        i += 1
    return lua_args, lua_infos
