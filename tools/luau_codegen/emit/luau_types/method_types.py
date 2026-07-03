from __future__ import annotations

from typing import TYPE_CHECKING, Dict, List

from luau_codegen.parse.broma import Class, Method

if TYPE_CHECKING:
    from luau_codegen.model.codegen_context import CodegenContext
from luau_codegen.convert.type_map import TypeInfo, classify_arg, classify_return
from luau_codegen.convert.sel_args import iter_lua_method_args

_DUMMY_CLS = Class(name="")

LUAU_KEYWORDS = frozenset(
    {
        "and",
        "break",
        "do",
        "else",
        "elseif",
        "end",
        "false",
        "for",
        "function",
        "if",
        "in",
        "local",
        "nil",
        "not",
        "or",
        "repeat",
        "return",
        "then",
        "true",
        "until",
        "while",
        "continue",
        "export",
        "type",
        "typeof",
        "declare",
    }
)


def lua_export_name(cpp_name: str, grouped: Dict[str, List[Method]] | None = None) -> str | None:
    if cpp_name not in LUAU_KEYWORDS:
        return cpp_name
    renamed = cpp_name + "ToLua"
    if grouped is not None and renamed in grouped:
        return None
    return renamed


def _method_return_type(
    cls: Class, m: Method, objects: Dict[str, Class], ctx: CodegenContext | None = None
) -> str:
    ret = classify_return(m.ret, objects, ctx=ctx)
    assert ret is not None
    out_types: List[str] = []
    if ret.kind != "void":
        out_types.append(ret.lua_type)
    arg_infos: List[TypeInfo] = []
    for arg in m.args:
        info = classify_arg(arg.type, objects, owner_class=cls.name, ctx=ctx)
        assert info is not None
        arg_infos.append(info)
    for lua_arg in iter_lua_method_args(m, arg_infos, ret_kind=ret.kind):
        if lua_arg.out_only:
            out_types.append(lua_arg.info.lua_type)
    if not out_types:
        return "()"
    if len(out_types) == 1:
        return out_types[0]
    return "(" + ", ".join(out_types) + ")"


def _classify_input_args(
    cls: Class, m: Method, objects: Dict[str, Class], ctx: CodegenContext | None = None
) -> List[TypeInfo]:
    ret = classify_return(m.ret, objects, ctx=ctx)
    assert ret is not None
    arg_infos = []
    for arg in m.args:
        info = classify_arg(arg.type, objects, owner_class=cls.name, ctx=ctx)
        assert info is not None
        arg_infos.append(info)
    return [
        lua_arg.info
        for lua_arg in iter_lua_method_args(m, arg_infos, ret_kind=ret.kind)
        if not lua_arg.out_only
    ]


def _method_type(
    cls: Class,
    methods: List[Method],
    objects: Dict[str, Class],
    ctx: CodegenContext | None = None,
) -> str:
    assert len(methods) == 1
    m = methods[0]
    args = _classify_input_args(cls, m, objects, ctx=ctx)
    ret_type = _method_return_type(cls, m, objects, ctx=ctx)
    params = []
    if not m.is_static:
        params.append(f"self: {cls.name}")
    for i, arg in enumerate(args, start=1):
        params.append(f"arg{i}: {arg.lua_type}")
    return f"(({', '.join(params)}) -> {ret_type})"


def _widened_method_type(
    cls: Class,
    methods: List[Method],
    objects: Dict[str, Class],
    *,
    static: bool,
    ctx: CodegenContext | None = None,
) -> str:
    arg_lists = [_classify_input_args(cls, m, objects, ctx=ctx) for m in methods]
    prefix: List[str] = []
    for i in range(min(len(a) for a in arg_lists)):
        types_at_i = {a[i].lua_type for a in arg_lists}
        if len(types_at_i) == 1:
            prefix.append(arg_lists[0][i].lua_type)
        else:
            break
    returns = {_method_return_type(cls, m, objects, ctx=ctx) for m in methods}
    ret = returns.pop() if len(returns) == 1 else "any?"
    params: List[str] = []
    if not static:
        params.append(f"self: {cls.name}")
    params += [f"arg{i}: {t}" for i, t in enumerate(prefix, start=1)]
    params.append("...any")
    return f"({', '.join(params)}) -> {ret}"
