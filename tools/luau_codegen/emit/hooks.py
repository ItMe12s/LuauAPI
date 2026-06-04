from __future__ import annotations

from typing import TYPE_CHECKING, Dict

if TYPE_CHECKING:
    from luau_codegen.model.codegen_context import CodegenContext

from luau_codegen.parse.broma import Class, Method
from luau_codegen.policy.hooks import hook_address_expr
from luau_codegen.convert.marshalling import emit_stack_check, push_value
from luau_codegen.model.domain import cxx_name, lua_namespace
from luau_codegen.convert.type_map import TypeInfo, classify_arg, classify_return
from luau_codegen.util.identifiers import cxx_id


def _cstr(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


def hook_id(cls: Class, m: Method) -> str:
    return f"{lua_namespace(cls)}.{cls.name}:{m.name}/{len(m.args)}"


def hook_suffix(cls: Class, m: Method) -> str:
    return f"{cxx_id(cls.name)}_{cxx_id(m.name)}_{len(m.args)}"


def _emit_apply_args_ctx(suffix: str, args: list[tuple]) -> str:
    if not args:
        return ""
    fields = []
    for _, info, name in args:
        if info.kind == "string" and info.cxx_type.endswith("*"):
            fields.append(f"        std::string* {name}Storage;")
            fields.append(f"        char const** {name};")
        else:
            fields.append(f"        {info.cxx_type}* {name};")
    body = "\n".join(fields)
    return f"    struct ApplyArgsCtx_{suffix} {{\n{body}\n    }};\n\n"


def _emit_apply_return_ctx(suffix: str, ret: TypeInfo) -> str:
    if ret.kind == "void":
        return ""
    return f"    struct ApplyReturnCtx_{suffix} {{\n        {ret.cxx_type}* result;\n    }};\n\n"


def _emit_apply_args_fn(
    suffix: str,
    args: list[tuple],
    named_args: bool,
) -> str:
    if not args:
        return (
            f"    int luaapi_apply_args_{suffix}(lua_State* L) {{\n"
            f'        if (!lua_istable(L, 1)) luaL_error(L, "hook args expected table");\n'
            f'        if (lua_objlen(L, 1) != 0) luaL_error(L, "hook args expected empty table");\n'
            f"        return 0;\n"
            f"    }}\n\n"
        )

    lines = [
        f"    int luaapi_apply_args_{suffix}(lua_State* L) {{\n",
        f"        auto* ctx = static_cast<ApplyArgsCtx_{suffix}*>(lua_tolightuserdata(L, lua_upvalueindex(1)));\n",
        '        if (!lua_istable(L, 1)) luaL_error(L, "hook args expected table");\n',
        "        int idx = 1;\n",
        f"        bool useArrayArgs = lua_objlen(L, idx) == {len(args)};\n",
        f"        bool useNamedArgs = {'true' if named_args else 'false'};\n",
        '        if (!useArrayArgs && !useNamedArgs) luaL_error(L, "hook args table shape invalid");\n',
    ]
    for i, (arg, info, name) in enumerate(args, start=1):
        tmp = f"{name}Override"
        lines.append(
            f'        if (useArrayArgs) lua_rawgeti(L, idx, {i}); else lua_getfield(L, idx, "{_cstr(arg.name)}");\n'
        )
        lines.extend(
            f"        {line.lstrip()}"
            for line in emit_stack_check(
                info,
                "-1",
                tmp,
                "hook args",
                allow_nil_object=(info.kind == "object"),
                declare=True,
            )
        )
        lines.append("        lua_pop(L, 1);\n")
    for _, info, name in args:
        tmp = f"{name}Override"
        if info.kind == "string" and info.cxx_type.endswith("*"):
            lines.append(f"        *ctx->{name}Storage = {tmp}_storage;\n")
            lines.append(f"        *ctx->{name} = ctx->{name}Storage->c_str();\n")
        else:
            lines.append(f"        *ctx->{name} = {tmp};\n")
    lines.append("        return 0;\n")
    lines.append("    }\n\n")
    return "".join(lines)


def _emit_apply_return_fn(suffix: str, ret: TypeInfo, label: str, fn_name: str) -> str:
    if ret.kind == "void":
        return f"    int {fn_name}(lua_State* L) {{\n        return 0;\n    }}\n\n"
    tmp = "valueOverride"
    lines = [
        f"    int {fn_name}(lua_State* L) {{\n",
        f"        auto* ctx = static_cast<ApplyReturnCtx_{suffix}*>(lua_tolightuserdata(L, lua_upvalueindex(1)));\n",
    ]
    lines.extend(
        f"        {line.lstrip()}" for line in emit_stack_check(ret, "1", tmp, label, declare=True)
    )
    lines.append(f"        *ctx->result = {tmp};\n")
    lines.append("        return 0;\n")
    lines.append("    }\n\n")
    return "".join(lines)


def emit_hook_target(
    cls: Class,
    m: Method,
    objects: Dict[str, Class],
    target_platform: str,
    ctx: CodegenContext | None = None,
) -> str:
    ret = classify_return(m.ret, objects, ctx=ctx)
    assert ret is not None
    args = []
    call_args = []
    for i, arg in enumerate(m.args):
        info = classify_arg(arg.type, objects, ctx=ctx)
        assert info is not None
        name = f"arg{i}"
        args.append((arg, info, name))
        call_args.append(name)

    suffix = hook_suffix(cls, m)
    target_id = hook_id(cls, m)
    cxx_cls = cxx_name(cls)
    ret_type = "void" if ret.kind == "void" else ret.cxx_type
    params = [f"{cxx_cls}* self"]
    params.extend(f"{info.cxx_type} {name}" for _, info, name in args)
    params_text = ", ".join(params)
    call = f"self->{m.name}({', '.join(call_args)})"
    arg_names = [arg.name for arg, _, _ in args]
    named_args = bool(arg_names) and len(set(arg_names)) == len(arg_names)
    apply_args_ctx = "&applyArgsCtx" if args else "nullptr"

    out: list[str] = []
    out.append(_emit_apply_args_ctx(suffix, args))
    out.append(_emit_apply_return_ctx(suffix, ret))
    out.append(_emit_apply_args_fn(suffix, args, named_args))
    if ret.kind != "void":
        out.append(
            _emit_apply_return_fn(suffix, ret, "hook return", f"luaapi_apply_return_{suffix}")
        )
        out.append(_emit_apply_return_fn(suffix, ret, "hook skip", f"luaapi_apply_skip_{suffix}"))

    out.append(f"    {ret_type} luaapi_hook_{suffix}({params_text}) {{\n")
    if ret.kind != "void":
        out.append(f"        {ret.cxx_type} result{{}};\n")
    for _, info, name in args:
        if info.kind == "string" and info.cxx_type.endswith("*"):
            out.append(f"        std::string {name}Storage;\n")
    out.append("        bool skipOriginal = false;\n")
    if args:
        ctx_fields = []
        for _, info, name in args:
            if info.kind == "string" and info.cxx_type.endswith("*"):
                ctx_fields.append(f"&{name}Storage")
                ctx_fields.append(f"&{name}")
            else:
                ctx_fields.append(f"&{name}")
        out.append(f"        ApplyArgsCtx_{suffix} applyArgsCtx{{ {', '.join(ctx_fields)} }};\n")
    if ret.kind != "void":
        out.append(f"        ApplyReturnCtx_{suffix} applyReturnCtx{{ &result }};\n")
    out.append(
        f'        skipOriginal = luauapi_gen::runLuaPreHooks("{target_id}", {1 + len(args)}, [&](lua_State* L) {{\n'
    )
    out.extend(
        f"    {line}"
        for line in push_value(TypeInfo("object", f"{cxx_cls}*", cls.name, cls.name), "self")
    )
    for _, info, name in args:
        out.extend(f"    {line}" for line in push_value(info, name))
    out.append("        }, [&](lua_State* L, int idx) -> bool {\n")
    out.append(
        f'            return luauapi_gen::applyHookOverride(L, idx, &luaapi_apply_args_{suffix}, {apply_args_ctx}, "{target_id}");\n'
    )
    out.append("        }, [&](lua_State* L, int idx) -> bool {\n")
    if ret.kind == "void":
        out.append("            return true;\n")
    else:
        out.append(
            f'            return luauapi_gen::applyHookOverride(L, idx, &luaapi_apply_skip_{suffix}, &applyReturnCtx, "{target_id}");\n'
        )
    out.append("        });\n")
    out.append("        if (!skipOriginal) {\n")
    if ret.kind == "void":
        out.append(f"            {call};\n")
    else:
        out.append(f"            result = {call};\n")
    out.append("        }\n")
    nargs = 1 + len(args) + (1 if ret.kind != "void" else 0)
    out.append(
        f'        luauapi_gen::runLuaPostHooks("{target_id}", {nargs}, [&](lua_State* L) {{\n'
    )
    out.extend(
        f"    {line}"
        for line in push_value(TypeInfo("object", f"{cxx_cls}*", cls.name, cls.name), "self")
    )
    for _, info, name in args:
        out.extend(f"    {line}" for line in push_value(info, name))
    if ret.kind != "void":
        out.extend(f"    {line}" for line in push_value(ret, "result"))
    out.append("        }, [&](lua_State* L, int idx) -> bool {\n")
    if ret.kind == "void":
        out.append("            return true;\n")
    else:
        out.append(
            f'            return luauapi_gen::applyHookOverride(L, idx, &luaapi_apply_return_{suffix}, &applyReturnCtx, "{target_id}");\n'
        )
    out.append("        });\n")
    if ret.kind != "void":
        out.append("        return result;\n")
    out.append("    }\n\n")
    address = hook_address_expr(cls, m, target_platform)
    out.append(
        f"    geode::Result<geode::Hook*> luaapi_create_hook_{suffix}(std::string const& displayName) {{\n"
    )
    out.append(f"        void* const address = {address};\n")
    out.append("        if (!address) {\n")
    out.append(
        f'            return geode::Err("hook address unresolved for {_cstr(target_id)}");\n'
    )
    out.append("        }\n")
    out.append(
        f"        return geode::Mod::get()->hook(address, "
        f"&luaapi_hook_{suffix}, displayName, tulip::hook::TulipConvention::Default);\n"
    )
    out.append("    }\n\n")
    return "".join(out)
