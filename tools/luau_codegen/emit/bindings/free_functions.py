from __future__ import annotations

from collections import defaultdict
from typing import TYPE_CHECKING, Dict, List, Optional

if TYPE_CHECKING:
    from luau_codegen.model.codegen_context import CodegenContext

from luau_codegen.model.free_fn_sources import free_function_includes
from luau_codegen.parse.broma import Class, Function
from luau_codegen.util import cxx_id
from luau_codegen.emit.bindings.invoke_common import (
    emit_invoke_return_tail,
    emit_invoke_void_tail,
    emit_lua_invoke_arg,
)
from luau_codegen.convert.sel_args import iter_lua_method_args
from luau_codegen.convert.type_map import (
    TypeInfo,
    require_classify_arg,
    require_classify_return,
)
from luau_codegen.emit.cxx_templates import file_preamble
from luau_codegen.emit.bindings.dispatch import emit_arity_dispatcher
from luau_codegen.emit.luau_types.manual_fields import manual_field_names_by_path

FREE_FUNCTIONS_FILE = "bindings_free_functions.cpp"


def _free_fn_base(fn: Function) -> str:
    return f"luaapi_fn_{cxx_id(fn.namespace)}_{cxx_id(fn.name)}"


def _emit_free_invoke(
    fn: Function,
    objects: Dict[str, Class],
    suffix: str,
    ctx: CodegenContext | None = None,
) -> str:
    cname = _free_fn_base(fn) + suffix
    label = f"{fn.lua_path}.{fn.name}"
    ret = require_classify_return(fn.ret, objects, ctx=ctx)
    arg_infos = [require_classify_arg(arg.type, objects, ctx=ctx) for arg in fn.args]
    input_count = sum(
        1
        for lua_arg in iter_lua_method_args(fn, arg_infos, ret_kind=ret.kind)
        if not lua_arg.out_only
    )

    out = [f"    int {cname}(lua_State* L) {{\n"]
    out.append(
        f'        if (lua_gettop(L) != {input_count}) luaL_error(L, "{label} expected {input_count} args");\n'
    )
    call_args: List[str] = []
    selector_handlers: List[tuple[str, str]] = []
    delegate_trampolines: List[str] = []
    lua_idx = 1
    for lua_arg in iter_lua_method_args(fn, arg_infos, ret_kind=ret.kind):
        lines, lua_idx = emit_lua_invoke_arg(
            lua_arg,
            lua_idx=lua_idx,
            label=label,
            call_args=call_args,
            selector_handlers=selector_handlers,
            delegate_trampolines=delegate_trampolines,
        )
        out.extend(lines)

    target = f"{fn.namespace}::{fn.name}({', '.join(call_args)})"
    out_refs = [
        (arg_idx, info)
        for arg_idx, (_, info) in enumerate(zip(fn.args, arg_infos))
        if ret.kind == "void" and info.is_out
    ]
    if ret.kind == "void":
        out.extend(
            emit_invoke_void_tail(
                target,
                ret=ret,
                out_refs=out_refs,
                selector_handlers=selector_handlers,
                delegate_trampolines=delegate_trampolines,
                is_static=True,
            )
        )
    else:
        out.extend(
            emit_invoke_return_tail(
                target,
                ret=ret,
                selector_handlers=selector_handlers,
                delegate_trampolines=delegate_trampolines,
                is_static=True,
            )
        )
    out.append("    }\n\n")
    return "".join(out)


def _emit_free_dispatcher(
    fns: List[Function], objects: Dict[str, Class], ctx: CodegenContext | None = None
) -> str:
    if len(fns) == 1:
        return ""
    base = _free_fn_base(fns[0])
    label = f"{fns[0].lua_path}.{fns[0].name}"
    cases = []
    for idx, fn in enumerate(fns):
        arg_infos = [require_classify_arg(arg.type, objects, ctx=ctx) for arg in fn.args]
        ret = require_classify_return(fn.ret, objects, ctx=ctx)
        input_count = sum(
            1
            for lua_arg in iter_lua_method_args(fn, arg_infos, ret_kind=ret.kind)
            if not lua_arg.out_only
        )
        cases.append((input_count, f"{base}_{idx}"))
    return emit_arity_dispatcher(base, "lua_gettop(L)", cases, label)


def emit_free_functions_file(
    functions: List[Function],
    objects: Dict[str, Class],
    ctx: CodegenContext | None = None,
    manual_fields: Optional[Dict[str, List[str]]] = None,
) -> str:
    manual_names = manual_field_names_by_path(manual_fields)
    by_key: dict[tuple[str, str], list[Function]] = defaultdict(list)
    for fn in functions:
        if fn.name in manual_names.get(fn.lua_path, frozenset()):
            continue
        by_key[(fn.namespace, fn.name)].append(fn)

    includes = "".join(f"#include <{header}>\n" for header in free_function_includes())
    out = [
        file_preamble(),
        includes + "\n",
        "namespace luauapi_gen::free_functions {\n\n",
        "namespace {\n\n",
    ]
    for fns in by_key.values():
        for idx, fn in enumerate(fns):
            suffix = "" if len(fns) == 1 else f"_{idx}"
            out.append(_emit_free_invoke(fn, objects, suffix, ctx=ctx))
        out.append(_emit_free_dispatcher(fns, objects, ctx=ctx))
    out.append("} // namespace\n\n")

    by_ns: dict[str, list[tuple[str, str]]] = defaultdict(list)
    for (_, name), fns in by_key.items():
        by_ns[fns[0].lua_path].append((name, _free_fn_base(fns[0])))

    out.append("geode::Result<void> bindFreeFunctions(lua_State* L) {\n")
    for lua_path, entries in by_ns.items():
        out.append(f'    luax::getOrCreateTable(L, "{lua_path}");\n')
        for name, base in entries:
            out.append(f'    lua_pushcfunction(L, &{base}, "{lua_path}.{name}");\n')
            out.append(f'    lua_setfield(L, -2, "{name}");\n')
        out.append("    lua_pop(L, 1);\n")
    out.append("    return geode::Ok();\n")
    out.append("}\n\n")
    out.append("} // namespace luauapi_gen::free_functions\n\n")
    out.append("namespace {\n")
    out.append(
        "    LUAX_BINDING_PRIORITY(GeneratedFreeFunctions, luauapi_gen::free_functions::bindFreeFunctions, 3)\n"
    )
    out.append("}\n")
    return "".join(out)
