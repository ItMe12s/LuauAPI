from __future__ import annotations

from collections import defaultdict
from typing import Dict, List

from luau_codegen.parse.broma import Class, Function
from luau_codegen.policy.free_functions import free_function_allowed
from luau_codegen.emit.cxx_templates import file_preamble
from luau_codegen.convert.marshalling import check_arg, push_return
from luau_codegen.convert.type_map import (
    classify_arg,
    classify_return,
    require_classify_arg,
    require_classify_return,
)
from luau_codegen.util.identifiers import cxx_id

FREE_FUNCTIONS_FILE = "bindings_free_functions.cpp"


def free_function_supported(fn: Function, objects: Dict[str, Class]) -> bool:
    if classify_return(fn.ret, objects) is None:
        return False
    return all(classify_arg(arg.type, objects) is not None for arg in fn.args)


def _free_fn_base(fn: Function) -> str:
    return f"luaapi_fn_{cxx_id(fn.namespace)}_{cxx_id(fn.name)}"


def _emit_free_invoke(fn: Function, objects: Dict[str, Class], suffix: str) -> str:
    cname = _free_fn_base(fn) + suffix
    label = f"{fn.lua_path}.{fn.name}"
    ret = require_classify_return(fn.ret, objects)
    arg_infos = [require_classify_arg(arg.type, objects) for arg in fn.args]

    out = [f"    int {cname}(lua_State* L) {{\n"]
    out.append(
        f'        if (lua_gettop(L) != {len(fn.args)}) luaL_error(L, "{label} expected {len(fn.args)} args");\n'
    )
    call_args: List[str] = []
    for arg_idx, (arg, info) in enumerate(zip(fn.args, arg_infos)):
        var = f"arg{arg_idx}"
        out.extend(check_arg(arg, info, arg_idx + 1, var, label))
        call_args.append(f"std::move({var})" if info.kind == "callback" else var)

    target = f"{fn.namespace}::{fn.name}({', '.join(call_args)})"
    if ret.kind == "void":
        out.append(f"        {target};\n")
        out.extend(push_return(ret, "", False))
    else:
        out.append(f"        auto result = {target};\n")
        out.extend(push_return(ret, "result", False))
    out.append("    }\n\n")
    return "".join(out)


def _emit_free_dispatcher(fns: List[Function], objects: Dict[str, Class]) -> str:
    if len(fns) == 1:
        return ""
    base = _free_fn_base(fns[0])
    label = f"{fns[0].lua_path}.{fns[0].name}"
    out = [f"    int {base}(lua_State* L) {{\n", "        switch (lua_gettop(L)) {\n"]
    for idx, fn in enumerate(fns):
        out.append(f"            case {len(fn.args)}: return {base}_{idx}(L);\n")
    out.append("            default: break;\n")
    out.append("        }\n")
    out.append(f'        luaL_error(L, "{label} unsupported overload arity");\n')
    out.append("    }\n\n")
    return "".join(out)


def emit_free_functions_file(
    functions: List[Function],
    objects: Dict[str, Class],
    target_platform: str = "win",
    skipped: list[tuple[str, str, str]] | None = None,
) -> str:
    by_key: dict[tuple[str, str], list[Function]] = defaultdict(list)
    for fn in functions:
        if not free_function_supported(fn, objects):
            continue
        if not free_function_allowed(fn, target_platform):
            if skipped is not None:
                skipped.append(
                    (
                        fn.lua_path,
                        fn.name,
                        f"free-function-override-arity:{target_platform}",
                    )
                )
            continue
        by_key[(fn.namespace, fn.name)].append(fn)

    out = [
        file_preamble(),
        "#include <Geode/ui/Popup.hpp>\n",
        "#include <Geode/utils/general.hpp>\n\n",
        "namespace luauapi_gen::free_functions {\n\n",
        "namespace {\n\n",
    ]
    for fns in by_key.values():
        for idx, fn in enumerate(fns):
            suffix = "" if len(fns) == 1 else f"_{idx}"
            out.append(_emit_free_invoke(fn, objects, suffix))
        out.append(_emit_free_dispatcher(fns, objects))
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
