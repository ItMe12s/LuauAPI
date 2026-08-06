from __future__ import annotations


from luau_codegen.convert.marshalling import (
    check_arg,
    check_sel_handler,
    push_value,
    sel_call_args,
    sel_selector_call_arg,
)
from luau_codegen.convert.sel_args import LuaMethodArg
from luau_codegen.convert.type_primitives import TypeInfo
from luau_codegen.policy.containers import CONTAINER_KINDS
from luau_codegen.emit.types_binding import emit_value_local_decl


def emit_out_arg(info: TypeInfo, var: str) -> tuple[list[str], str]:
    if info.kind == "value":
        return [f"        {emit_value_local_decl(info, var)}\n"], var
    if info.kind == "enum":
        return [f"        {info.cxx_type} {var}{{}};\n"], var
    if info.kind in CONTAINER_KINDS:
        return [f"        {info.cxx_type} {var}{{}};\n"], (f"&{var}" if info.is_vector_ptr else var)
    return [f"        {info.cxx_type} {var}{{}};\n"], var


def emit_lua_invoke_arg(
    lua_arg: LuaMethodArg,
    *,
    lua_idx: int,
    label: str,
    call_args: list[str],
    selector_handlers: list[str],
    delegate_trampolines: list[str] | None = None,
) -> tuple[list[str], int]:
    out: list[str] = []
    info = lua_arg.info
    arg_idx = lua_arg.arg_index
    var = f"arg{arg_idx}"
    trampolines = delegate_trampolines if delegate_trampolines is not None else []

    if lua_arg.out_only:
        lines, call_expr = emit_out_arg(info, var)
        out.extend(lines)
        call_args.append(call_expr)
        return out, lua_idx

    if lua_arg.orphan:
        sel_var = f"sel{arg_idx}"
        out.extend(check_sel_handler(lua_idx, sel_var, info, label))
        call_args.append(sel_selector_call_arg(info))
        selector_handlers.append(f"{sel_var}_handler")
        return out, lua_idx + 1

    if lua_arg.sel_pair and not lua_arg.implicit_self_target:
        sel_var = f"sel{arg_idx}"
        out.extend(check_sel_handler(lua_idx, sel_var, info, label))
        call_args.extend(sel_call_args(sel_var, info, handler_first=lua_arg.handler_first))
        selector_handlers.append(f"{sel_var}_handler")
        return out, lua_idx + 1

    out.extend(check_arg(lua_arg.arg, info, lua_idx, var, label))
    if info.kind in ("callback", "task_handle", "optional_task_handle"):
        call_args.append(f"std::move({var})")
    elif info.kind == "delegate":
        call_args.append(var)
        trampolines.append(f"{var}_trampoline")
    else:
        call_args.append(var)
    return out, lua_idx + 1


def emit_return_value_push(
    ret: TypeInfo,
    expr: str,
    *,
    owned_return: bool = False,
    ref_owner_expr: str | None = None,
) -> list[str]:
    if ret.kind != "vector_view":
        return push_value(ret, expr, owned_return)
    if ref_owner_expr is not None:
        return push_value(ret, expr, False, owner_expr=ref_owner_expr)
    return push_value(ret, expr, False, vector_owned=True)


def emit_trampoline_registrations(
    selector_handlers: list[str],
    delegate_trampolines: list[str],
    *,
    ret_kind: str,
    is_static: bool,
) -> list[str]:
    if not selector_handlers and not delegate_trampolines:
        return []
    lines: list[str] = []
    if ret_kind == "object":
        for handler in selector_handlers:
            lines.append(f"        luax::anchorTrampoline(result, {handler});\n")
        for trampoline in delegate_trampolines:
            lines.append(f"        luax::anchorDelegate(result, {trampoline});\n")
    elif not is_static:
        for handler in selector_handlers:
            lines.append(f"        luax::anchorTrampoline(self, {handler});\n")
        for trampoline in delegate_trampolines:
            lines.append(f"        luax::anchorDelegate(self, {trampoline});\n")
    else:
        for handler in selector_handlers:
            lines.append(f"        luax::registerOrphanTrampoline({handler});\n")
        for trampoline in delegate_trampolines:
            lines.append(f"        luax::registerOrphanTrampoline({trampoline});\n")
    return lines


def emit_invoke_void_tail(
    target: str,
    *,
    ret: TypeInfo,
    out_refs: list[tuple[int, TypeInfo]],
    selector_handlers: list[str],
    delegate_trampolines: list[str] | None = None,
    is_static: bool,
) -> list[str]:
    lines = [f"        {target};\n"]
    lines.extend(
        emit_trampoline_registrations(
            selector_handlers,
            delegate_trampolines or [],
            ret_kind=ret.kind,
            is_static=is_static,
        )
    )
    if out_refs:
        for arg_idx, oinfo in out_refs:
            lines.extend(
                push_value(
                    oinfo,
                    f"arg{arg_idx}",
                    False,
                    vector_owned=oinfo.kind == "vector_view",
                )
            )
        lines.append(f"        return {len(out_refs)};\n")
    else:
        lines.append("        return 0;\n")
    return lines


def emit_invoke_return_tail(
    target: str,
    *,
    ret: TypeInfo,
    selector_handlers: list[str],
    delegate_trampolines: list[str] | None = None,
    is_static: bool,
    owned_return: bool = False,
    ref_owner_expr: str | None = None,
    pre_push_lines: list[str] | None = None,
    out_refs: list[tuple[int, TypeInfo]] | None = None,
) -> list[str]:
    lines = [f"        auto result = {target};\n"]
    if pre_push_lines:
        lines.extend(pre_push_lines)
    lines.extend(
        emit_trampoline_registrations(
            selector_handlers,
            delegate_trampolines or [],
            ret_kind=ret.kind,
            is_static=is_static,
        )
    )
    push_lines = emit_return_value_push(
        ret,
        "result",
        owned_return=owned_return,
        ref_owner_expr=ref_owner_expr,
    )
    if out_refs:
        lines.extend(push_lines)
        for arg_idx, oinfo in out_refs:
            lines.extend(
                push_value(
                    oinfo,
                    f"arg{arg_idx}",
                    False,
                    vector_owned=oinfo.kind == "vector_view",
                )
            )
        lines.append(f"        return {1 + len(out_refs)};\n")
    else:
        lines.extend(push_lines)
        lines.append("        return 1;\n")
    return lines
