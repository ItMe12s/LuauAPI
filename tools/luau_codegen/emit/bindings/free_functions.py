from __future__ import annotations

from collections import defaultdict
from typing import TYPE_CHECKING, Dict, List

if TYPE_CHECKING:
    from luau_codegen.model.codegen_context import CodegenContext

from luau_codegen.parse.broma import Class, Function
from luau_codegen.util.identifiers import cxx_id
from luau_codegen.convert.marshalling import (
    check_arg,
    check_sel_handler,
    emit_stack_check,
    push_return,
    push_value,
    sel_call_args,
    sel_selector_call_arg,
)
from luau_codegen.convert.sel_args import iter_lua_method_args
from luau_codegen.convert.type_map import (
    TypeInfo,
    require_classify_arg,
    require_classify_return,
)
from luau_codegen.emit.cxx_templates import file_preamble

FREE_FUNCTIONS_FILE = "bindings_free_functions.cpp"


def _free_fn_base(fn: Function) -> str:
    return f"luaapi_fn_{cxx_id(fn.namespace)}_{cxx_id(fn.name)}"


def _emit_vector_return_push(ret: TypeInfo, expr: str) -> list[str]:
    if ret.kind != "vector_view":
        return push_return(ret, expr, False)
    return push_return(ret, expr, False, vector_owned=True)


def _emit_vector_out_push(info: TypeInfo, var: str) -> list[str]:
    if info.kind == "vector_view":
        return push_value(info, var, False, vector_owned=True)
    return push_value(info, var, False)


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
    lua_idx = 1
    for lua_arg in iter_lua_method_args(fn, arg_infos, ret_kind=ret.kind):
        info = lua_arg.info
        arg_idx = lua_arg.arg_index
        var = f"arg{arg_idx}"
        if lua_arg.out_only:
            if info.kind == "vector_view":
                out.append(f"        {info.cxx_type} {var}{{}};\n")
                call_args.append(f"&{var}" if info.is_vector_ptr else var)
            else:
                out.append(f"        {info.cxx_type} {var}{{}};\n")
                call_args.append(var)
            continue
        if lua_arg.orphan:
            sel_var = f"sel{arg_idx}"
            out.extend(check_sel_handler(lua_idx, sel_var, info, label))
            call_args.append(sel_selector_call_arg(info))
            selector_handlers.append((f"{sel_var}_handler", info.class_name or "menu"))
            lua_idx += 1
            continue
        if lua_arg.sel_pair and not lua_arg.implicit_self_target:
            sel_var = f"sel{arg_idx}"
            out.extend(check_sel_handler(lua_idx, sel_var, info, label))
            call_args.extend(
                sel_call_args(sel_var, info, handler_first=lua_arg.handler_first)
            )
            selector_handlers.append((f"{sel_var}_handler", info.class_name or "menu"))
            lua_idx += 1
            continue
        if lua_arg.orphan:
            sel_var = f"sel{arg_idx}"
            out.extend(check_sel_handler(lua_idx, sel_var, info, label))
            call_args.append(sel_selector_call_arg(info))
            selector_handlers.append((f"{sel_var}_handler", info.class_name or "menu"))
            lua_idx += 1
            continue
        out.extend(check_arg(lua_arg.arg, info, lua_idx, var, label))
        call_args.append(f"std::move({var})" if info.kind == "callback" else var)
        lua_idx += 1

    target = f"{fn.namespace}::{fn.name}({', '.join(call_args)})"
    out_refs = [
        (arg_idx, info)
        for arg_idx, (_, info) in enumerate(zip(fn.args, arg_infos))
        if ret.kind == "void" and info.is_out
    ]
    if ret.kind == "void":
        out.append(f"        {target};\n")
        for handler, variant in selector_handlers:
            if variant == "menu":
                out.append(f"        luax::registerOrphanMenuHandler({handler});\n")
            else:
                out.append(f"        luax::registerOrphanTrampoline({handler});\n")
        if out_refs:
            for arg_idx, info in out_refs:
                out.extend(_emit_vector_out_push(info, f"arg{arg_idx}"))
            out.append(f"        return {len(out_refs)};\n")
        else:
            out.extend(push_return(ret, "", False))
    else:
        out.append(f"        auto result = {target};\n")
        for handler, variant in selector_handlers:
            if ret.kind == "object" and variant == "menu":
                out.append(f"        luax::anchorMenuHandler(result, {handler});\n")
            elif ret.kind == "object":
                out.append(f"        luax::anchorTrampoline(result, {handler});\n")
            elif variant == "menu":
                out.append(f"        luax::registerOrphanMenuHandler({handler});\n")
            else:
                out.append(f"        luax::registerOrphanTrampoline({handler});\n")
        out.extend(_emit_vector_return_push(ret, "result"))
    out.append("    }\n\n")
    return "".join(out)


def _emit_free_dispatcher(
    fns: List[Function], objects: Dict[str, Class], ctx: CodegenContext | None = None
) -> str:
    if len(fns) == 1:
        return ""
    base = _free_fn_base(fns[0])
    label = f"{fns[0].lua_path}.{fns[0].name}"
    out = [f"    int {base}(lua_State* L) {{\n", "        switch (lua_gettop(L)) {\n"]
    for idx, fn in enumerate(fns):
        arg_infos = [
            require_classify_arg(arg.type, objects, ctx=ctx) for arg in fn.args
        ]
        ret = require_classify_return(fn.ret, objects, ctx=ctx)
        input_count = sum(
            1
            for lua_arg in iter_lua_method_args(fn, arg_infos, ret_kind=ret.kind)
            if not lua_arg.out_only
        )
        out.append(f"            case {input_count}: return {base}_{idx}(L);\n")
    out.append("            default: break;\n")
    out.append("        }\n")
    out.append(f'        luaL_error(L, "{label} unsupported overload arity");\n')
    out.append("    }\n\n")
    return "".join(out)


def emit_free_functions_file(
    functions: List[Function],
    objects: Dict[str, Class],
    ctx: CodegenContext | None = None,
) -> str:
    by_key: dict[tuple[str, str], list[Function]] = defaultdict(list)
    for fn in functions:
        by_key[(fn.namespace, fn.name)].append(fn)

    out = [
        file_preamble(),
        "#include <Geode/ui/Popup.hpp>\n",
        "#include <Geode/ui/GeodeUI.hpp>\n",
        "#include <Geode/utils/general.hpp>\n",
        "#include <Geode/utils/string.hpp>\n",
        "#include <Geode/utils/random.hpp>\n\n",
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
