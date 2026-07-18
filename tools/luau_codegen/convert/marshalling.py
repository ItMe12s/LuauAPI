from __future__ import annotations

from luau_codegen.parse.broma import Arg
from luau_codegen.convert.type_map import (
    COMPOSITE_KINDS,
    TypeInfo,
    UNSIGNED_NUMERIC_TYPES,
    VALUE_CHECK_CXX_TYPES,
    WIDE_INTEGER_TYPES,
    is_sel_type,
    sel_variant as sel_variant_name,
)
from luau_codegen.model import delegate_specs as _delegate_specs
from luau_codegen.model.nested_containers import AUDITED_POINTER_GRID_KIND
from luau_codegen.emit.types_binding import emit_value_default_expr, emit_value_local_decl


def _prefix(declare: bool, var: str) -> str:
    return f"auto {var}" if declare else var


def _task_handle_inner_cxx(info: TypeInfo) -> str:
    if info.element_type is None:
        raise ValueError("task handle requires result type")
    return info.element_type.cxx_type


def _luax_numeric_check_type(cxx: str) -> str:
    if cxx in ("float", "double"):
        return cxx
    if cxx in WIDE_INTEGER_TYPES:
        return "double"
    if cxx in UNSIGNED_NUMERIC_TYPES:
        return "unsigned"
    return "int"


def _vector_view_pointee_cxx(info: TypeInfo) -> str:
    if info.element_type is None:
        raise ValueError("vector view requires element type")
    return info.element_type.cxx_type[:-1]


def _vector_view_check_fn(info: TypeInfo) -> str:
    pointee = _vector_view_pointee_cxx(info)
    if info.element_type and info.element_type.kind == "opaque_handle":
        return f"checkOpaqueVectorView<{pointee}>"
    return f"checkObjectVectorView<{pointee}>"


def _cc_c_array_view_pointee_cxx(info: TypeInfo) -> str:
    if info.element_type is None:
        raise ValueError("ccCArray view requires element type")
    return info.element_type.cxx_type[:-1]


def _cc_c_array_view_push_fn(info: TypeInfo) -> str:
    pointee = _cc_c_array_view_pointee_cxx(info)
    return f"pushReadOnlyCCArrayView<{pointee}>"


def _vector_view_push_fn(info: TypeInfo, *, owned: bool, has_owner: bool) -> str:
    pointee = _vector_view_pointee_cxx(info)
    if info.element_type and info.element_type.kind == "opaque_handle":
        if owned or not has_owner:
            return f"pushOwnedReadOnlyOpaqueVectorView<{pointee}>"
        return f"pushReadOnlyOpaqueVectorView<{pointee}>"
    if owned or not has_owner:
        return f"pushOwnedReadOnlyVectorView<{pointee}>"
    return f"pushReadOnlyVectorView<{pointee}>"


def emit_stack_check(
    info: TypeInfo,
    idx: str | int,
    var: str,
    label: str,
    *,
    allow_nil_object: bool = False,
    declare: bool = True,
) -> list[str]:
    cxx = info.cxx_type
    if info.kind == "void":
        return []
    if info.kind == "bool":
        return [f'        {_prefix(declare, var)} = luax::check<bool>(L, {idx}, "{label}");\n']
    if info.kind == "seed_value":
        return [f'        {_prefix(declare, var)} = luax::check<int>(L, {idx}, "{label}");\n']
    if info.kind in ("number", "enum"):
        check_type = _luax_numeric_check_type(cxx)
        if check_type in ("float", "double"):
            return [
                f'        {_prefix(declare, var)} = luax::check<{check_type}>(L, {idx}, "{label}");\n'
            ]
        return [
            f'        {_prefix(declare, var)} = static_cast<{cxx}>(luax::check<{check_type}>(L, {idx}, "{label}"));\n'
        ]
    if info.kind == "wideint":
        return [
            f'        {_prefix(declare, var)} = luax::checkIntegerString<{cxx}>(L, {idx}, "{label}");\n'
        ]
    if info.kind == "string":
        if cxx == "char const*" or cxx == "const char*":
            storage = f"{var}_storage"
            lines = [
                f'        auto {storage} = luax::check<std::string>(L, {idx}, "{label}");\n',
            ]
            if declare:
                lines.append(f"        auto {var} = {storage}.c_str();\n")
            else:
                lines.append(f"        {var} = {storage}.c_str();\n")
            return lines
        if cxx == "gd::string":
            storage = f"{var}_storage"
            lines = [
                f'        auto {storage} = luax::check<std::string>(L, {idx}, "{label}");\n',
            ]
            if declare:
                lines.append(f"        auto {var} = gd::string({storage}.c_str());\n")
            else:
                lines.append(f"        {var} = gd::string({storage}.c_str());\n")
            return lines
        return [
            f'        {_prefix(declare, var)} = luax::check<std::string>(L, {idx}, "{label}");\n'
        ]
    if info.kind == "value":
        value_check = VALUE_CHECK_CXX_TYPES.get(info.lua_type)
        if value_check is None:
            raise ValueError(f"unsupported value type: {info.lua_type}")
        return [
            f'        {_prefix(declare, var)} = luax::check<{value_check}>(L, {idx}, "{label}");\n'
        ]
    if info.kind == "opaque_handle":
        cxx = info.cxx_type
        pointee = cxx[:-1] if cxx.endswith("*") else cxx
        if allow_nil_object:
            lines = [f"        {cxx} {var};\n"]
            lines.extend(
                [
                    f"        if (lua_isnil(L, {idx})) {{\n",
                    f"            {var} = nullptr;\n",
                    "        } else {\n",
                    f'            {var} = luax::checkOpaqueHandle<{pointee}>(L, {idx}, "{label}");\n',
                    "        }\n",
                ]
            )
            return lines
        return [
            f'        {_prefix(declare, var)} = luax::checkOpaqueHandle<{pointee}>(L, {idx}, "{label}");\n',
        ]
    if info.kind == "object":
        obj_type = info.cxx_type[:-1]
        if allow_nil_object:
            lines = [f"        {obj_type}* {var};\n"]
            lines.extend(
                [
                    f"        if (lua_isnil(L, {idx})) {{\n",
                    f"            {var} = nullptr;\n",
                    "        } else {\n",
                    f'            {var} = luax::Usertype<{obj_type}>::check(L, {idx}, "{label}");\n',
                    "        }\n",
                ]
            )
            return lines
        return [
            f'        {_prefix(declare, var)} = luax::Usertype<{obj_type}>::check(L, {idx}, "{label}");\n'
        ]
    if info.kind == "vector_view":
        if info.element_type is None or info.element_type.kind not in (
            "object",
            "opaque_handle",
        ):
            raise ValueError("vector view requires object or opaque element type")
        check_fn = _vector_view_check_fn(info)
        return [f'        {_prefix(declare, var)} = luax::{check_fn}(L, {idx}, "{label}");\n']
    if info.kind == AUDITED_POINTER_GRID_KIND:
        return [
            f'        {_prefix(declare, var)} = luax::checkAuditedPointerGrid<{info.cxx_type}>(L, {idx}, "{label}");\n'
        ]
    if info.kind in COMPOSITE_KINDS:
        return [
            f'        {_prefix(declare, var)} = luax::checkContainerValue<{info.cxx_type}>(L, {idx}, "{label}");\n'
        ]
    raise ValueError(f"unsupported type kind: {info.kind}")


def _push_impl(
    info: TypeInfo,
    expr: str,
    owned: bool,
    *,
    indent: str = "        ",
    owner_expr: str | None = None,
    vector_owned: bool = False,
) -> list[str]:
    if info.kind == "bool":
        return [f"{indent}luax::push(L, {expr});\n"]
    if info.kind == "seed_value":
        return [f"{indent}lua_pushnumber(L, static_cast<double>(static_cast<int>({expr})));\n"]
    if info.kind == "number":
        return [f"{indent}lua_pushnumber(L, static_cast<double>({expr}));\n"]
    if info.kind == "wideint":
        return [f"{indent}luax::pushIntegerString(L, {expr});\n"]
    if info.kind == "enum":
        return [f"{indent}lua_pushnumber(L, static_cast<double>(static_cast<int>({expr})));\n"]
    if info.kind == "string":
        if info.cxx_type.endswith("*"):
            return [f"{indent}luax::push(L, {expr});\n"]
        return [f"{indent}luax::push(L, std::string({expr}));\n"]
    if info.kind == "value":
        if info.lua_type in VALUE_CHECK_CXX_TYPES:
            return [f"{indent}luax::push(L, {expr});\n"]
    if info.kind in ("task_handle", "optional_task_handle"):
        return _push_task_handle(info, expr, indent=indent)
    if info.kind == "opaque_handle":
        return [
            f"{indent}luax::pushOpaqueHandle(L, {expr});\n",
        ]
    if info.kind == "object":
        if info.class_name == "CCNode":
            push = "pushOwnedDynamic" if owned else "pushBorrowedDynamic"
            return [f"{indent}luax::Usertype<cocos2d::CCNode>::{push}(L, {expr});\n"]
        if info.class_name == "CCObject":
            push = "pushOwnedDynamic" if owned else "pushBorrowedDynamic"
            return [f"{indent}luax::Usertype<cocos2d::CCObject>::{push}(L, {expr});\n"]
        push = "pushOwned" if owned else "pushBorrowed"
        return [f"{indent}luax::Usertype<{info.cxx_type[:-1]}>::{push}(L, {expr});\n"]
    if info.kind == AUDITED_POINTER_GRID_KIND:
        owner_arg = f", {owner_expr}" if owner_expr else ""
        return [f"{indent}luax::pushAuditedPointerGrid(L, {expr}{owner_arg});\n"]
    if info.kind == "cc_c_array_view":
        if info.element_type is None or info.element_type.kind != "object":
            raise ValueError("ccCArray view requires object element type")
        push_fn = _cc_c_array_view_push_fn(info)
        if not owner_expr:
            raise ValueError("ccCArray view push requires owner expression")
        return [f"{indent}luax::{push_fn}(L, {expr}, {owner_expr});\n"]
    if info.kind == "vector_view":
        if info.element_type is None or info.element_type.kind not in (
            "object",
            "opaque_handle",
        ):
            raise ValueError("vector view requires object or opaque element type")
        push_fn = _vector_view_push_fn(info, owned=vector_owned, has_owner=bool(owner_expr))
        if vector_owned or not owner_expr:
            return [f"{indent}luax::{push_fn}(L, {expr});\n"]
        return [f"{indent}luax::{push_fn}(L, {expr}, {owner_expr});\n"]
    if info.kind in COMPOSITE_KINDS:
        owner_arg = f", {owner_expr}" if owner_expr else ""
        return [f"{indent}luax::pushContainerValue<{info.cxx_type}>(L, {expr}{owner_arg});\n"]
    if info.kind == "delegate":
        return [f"{indent}luax::tryPushBoundDelegateTable(L, {expr});\n"]
    if info.kind == "result":
        return [
            f"{indent}if ({expr}.isOk()) {{\n",
            f"{indent}    lua_pushboolean(L, true);\n",
            f"{indent}}} else {{\n",
            f"{indent}    luax::push(L, {expr}.unwrapErr());\n",
            f"{indent}}}\n",
        ]
    raise ValueError(f"unsupported type kind: {info.kind}")


def _task_handle_pusher_arg(info: TypeInfo, indent: str) -> list[str]:
    inner = info.element_type
    if inner is None:
        raise ValueError("task handle requires result type")
    if inner.kind == "void":
        return ["nullptr"]
    lines = [
        "+[](lua_State* L, void const* raw) {\n",
        f"{indent}    auto const& value = *static_cast<{inner.cxx_type} const*>(raw);\n",
    ]
    lines.extend(_push_impl(inner, "value", False, indent=f"{indent}    "))
    lines.append(f"{indent}}}")
    return lines


def _push_task_handle(info: TypeInfo, expr: str, *, indent: str) -> list[str]:
    inner = _task_handle_inner_cxx(info)
    fn = (
        "pushOptionalGeodeTaskHandle"
        if info.kind == "optional_task_handle"
        else "pushGeodeTaskHandle"
    )
    pusher = _task_handle_pusher_arg(info, indent)
    if len(pusher) == 1:
        return [f"{indent}luax::{fn}<{inner}>(L, std::move({expr}), {pusher[0]});\n"]
    lines = [f"{indent}luax::{fn}<{inner}>(L, std::move({expr}), "]
    lines.extend(pusher)
    lines.append(");\n")
    return lines


def _callback_return_type(info: TypeInfo) -> str:
    ret = info.callback_ret
    if ret is None or ret.kind == "void":
        return "void"
    return ret.cxx_type


def _emit_callback_pop(var: str, ret: TypeInfo) -> list[str]:
    if ret.kind == "void":
        return []
    if ret.kind == "bool":
        return [
            "                +[](lua_State* L, void* raw) {\n",
            f'                    *static_cast<bool*>(raw) = luax::check<bool>(L, -1, "{var} callback return");\n',
            "                },\n",
        ]
    if ret.kind == "seed_value":
        cxx = ret.cxx_type
        return [
            "                +[](lua_State* L, void* raw) {\n",
            f'                    *static_cast<{cxx}*>(raw) = luax::check<int>(L, -1, "{var} callback return");\n',
            "                },\n",
        ]
    if ret.kind == "number":
        cxx = ret.cxx_type
        check = _luax_numeric_check_type(cxx)
        return [
            "                +[](lua_State* L, void* raw) {\n",
            f'                    *static_cast<{cxx}*>(raw) = static_cast<{cxx}>(luax::check<{check}>(L, -1, "{var} callback return"));\n',
            "                },\n",
        ]
    if ret.kind == "enum":
        return [
            "                +[](lua_State* L, void* raw) {\n",
            f'                    *static_cast<{ret.cxx_type}*>(raw) = static_cast<{ret.cxx_type}>(static_cast<int>(luax::check<int>(L, -1, "{var} callback return")));\n',
            "                },\n",
        ]
    if ret.kind == "object":
        obj = ret.cxx_type[:-1]
        return [
            "                +[](lua_State* L, void* raw) {\n",
            f"                    auto* slot = static_cast<{ret.cxx_type}*>(raw);\n",
            "                    if (lua_isnil(L, -1)) {\n",
            "                        *slot = nullptr;\n",
            "                    } else {\n",
            f'                        *slot = luax::Usertype<{obj}>::check(L, -1, "{var} callback return");\n',
            "                    }\n",
            "                },\n",
        ]
    if ret.kind == "opaque_handle":
        pointee = ret.cxx_type[:-1] if ret.cxx_type.endswith("*") else ret.cxx_type
        return [
            "                +[](lua_State* L, void* raw) {\n",
            f"                    auto* slot = static_cast<{ret.cxx_type}*>(raw);\n",
            "                    if (lua_isnil(L, -1)) {\n",
            "                        *slot = nullptr;\n",
            "                    } else {\n",
            f'                        *slot = luax::checkOpaqueHandle<{pointee}>(L, -1, "{var} callback return");\n',
            "                    }\n",
            "                },\n",
        ]
    if ret.kind == "string":
        store = "std::string" if ret.cxx_type in ("char const*", "const char*") else ret.cxx_type
        return [
            "                +[](lua_State* L, void* raw) {\n",
            f'                    *static_cast<{store}*>(raw) = luax::check<std::string>(L, -1, "{var} callback return");\n',
            "                },\n",
        ]
    if ret.kind == "value":
        check_type = VALUE_CHECK_CXX_TYPES.get(ret.lua_type)
        if check_type is None:
            raise ValueError(
                f"unsupported callback return value type {ret.lua_type!r} ({ret.cxx_type})"
            )
        return [
            "                +[](lua_State* L, void* raw) {\n",
            f'                    *static_cast<{ret.cxx_type}*>(raw) = luax::check<{check_type}>(L, -1, "{var} callback return");\n',
            "                },\n",
        ]
    raise ValueError(
        f"unsupported callback return kind {ret.kind!r} for {ret.cxx_type} ({ret.lua_type})"
    )


def _emit_invoke_failure_return(ret: TypeInfo) -> str:
    if ret.kind == "void":
        return ""
    if ret.kind == "bool":
        return "false"
    if ret.kind == "object":
        return "nullptr"
    if ret.kind == "number":
        cxx = ret.cxx_type
        if cxx in ("float", "double"):
            suffix = "f" if cxx == "float" else ""
            return f"static_cast<{cxx}>(0{suffix})"
        return f"static_cast<{cxx}>(0)"
    if ret.kind == "enum":
        return f"static_cast<{ret.cxx_type}>(0)"
    if ret.kind == "seed_value":
        return f"{ret.cxx_type}{{}}"
    if ret.kind == "value":
        return emit_value_default_expr(ret.cxx_type)
    if ret.kind == "string":
        return "std::string()"
    raise ValueError(
        f"unsupported callback return kind {ret.kind!r} for {ret.cxx_type} ({ret.lua_type})"
    )


def _emit_invoke_failure_handler(label: str, ret: TypeInfo) -> list[str]:
    lines = [f'                geode::log::error("{label} callback failed");\n']
    if ret.kind == "void":
        lines.append("                return;\n")
        return lines
    lines.append(f"                return {_emit_invoke_failure_return(ret)};\n")
    return lines


def _emit_callback_lambda(idx: int, var: str, info: TypeInfo, label: str) -> list[str]:
    ret = info.callback_ret or TypeInfo("void", "void", "()")
    ret_type = _callback_return_type(info)
    params = ", ".join(f"{cb.cxx_type} {var}_p{i}" for i, cb in enumerate(info.callback_args))
    lines = [
        f"        luaL_checktype(L, {idx}, LUA_TFUNCTION);\n",
        f"        auto {var}_cb = std::make_shared<luax::LuaCallback>(L, {idx});\n",
    ]
    if ret_type == "void":
        lines.append(f"        auto {var} = [{var}_cb]({params}) {{\n")
    else:
        lines.append(f"        auto {var} = [{var}_cb]({params}) -> {ret_type} {{\n")
        if ret.kind == "value":
            lines.append(f"            {emit_value_local_decl(ret.cxx_type, f'{var}_ret')}\n")
        else:
            lines.append(f"            {ret_type} {var}_ret{{}};\n")
    if info.callback_args:
        ctx_fields = "; ".join(f"{cb.cxx_type} p{i}" for i, cb in enumerate(info.callback_args))
        ctx_init = ", ".join(f"{var}_p{i}" for i in range(len(info.callback_args)))
        lines.append(f"            struct {var}_Ctx {{ {ctx_fields}; }};\n")
        lines.append(f"            {var}_Ctx ctx{{ {ctx_init} }};\n")
        nresults = 0 if ret.kind == "void" else 1
        lines.append(
            f"            if (!{var}_cb->invoke({len(info.callback_args)}, {nresults}, "
            f'"{label} callback", luax::kHookScriptDeadlineMs,\n'
        )
        lines.append("                +[](lua_State* L, void* raw) {\n")
        lines.append(f"                    auto* c = static_cast<{var}_Ctx*>(raw);\n")
        for i, cb in enumerate(info.callback_args):
            lines.extend(_push_impl(cb, f"c->p{i}", False, indent="                    "))
        lines.append("                }, &ctx")
        if ret.kind != "void":
            lines.append(",\n")
            lines.extend(_emit_callback_pop(var, ret))
            lines.append(f"                &{var}_ret")
        lines.append(")) {\n")
        lines.extend(_emit_invoke_failure_handler(label, ret))
        lines.append("            }\n")
    else:
        nresults = 0 if ret.kind == "void" else 1
        lines.append(
            f"            if (!{var}_cb->invoke(0, {nresults}, "
            f'"{label} callback", luax::kHookScriptDeadlineMs'
        )
        if ret.kind != "void":
            lines.append(", nullptr, nullptr")
            lines.extend(_emit_callback_pop(var, ret))
            lines.append(f", &{var}_ret")
        lines.append(")) {\n")
        lines.extend(_emit_invoke_failure_handler(label, ret))
        lines.append("            }\n")
    if ret_type != "void":
        lines.append(f"            return {var}_ret;\n")
    lines.append("        };\n")
    return lines


_SEL_VARIANTS: dict[str, tuple[str, str, str]] = {
    "menu": (
        "LuaMenuHandler",
        "LuaMenuHandler::create",
        "menu_selector(luax::LuaMenuHandler::onCallback)",
    ),
    "schedule": (
        "LuaScheduleHandler",
        "LuaScheduleHandler::create",
        "schedule_selector(luax::LuaScheduleHandler::onSchedule)",
    ),
    "callfunc": (
        "LuaCallFuncHandler",
        "LuaCallFuncHandler::create",
        "callfunc_selector(luax::LuaCallFuncHandler::onCallFunc)",
    ),
    "callfuncn": (
        "LuaCallFuncNHandler",
        "LuaCallFuncNHandler::create",
        "callfuncN_selector(luax::LuaCallFuncNHandler::onCallFuncN)",
    ),
    "callfuncnd": (
        "LuaCallFuncNDHandler",
        "LuaCallFuncNDHandler::create",
        "callfuncND_selector(luax::LuaCallFuncNDHandler::onCallFuncND)",
    ),
    "callfunco": (
        "LuaCallFuncOHandler",
        "LuaCallFuncOHandler::create",
        "callfuncO_selector(luax::LuaCallFuncOHandler::onCallFuncO)",
    ),
}


def _sel_variant(info: TypeInfo) -> tuple[str, str, str]:
    variant = info.class_name or (
        sel_variant_name(info.cxx_type) if is_sel_type(info.cxx_type) else None
    )
    if not variant or variant not in _SEL_VARIANTS:
        raise ValueError(
            f"unknown SEL variant for {info.cxx_type!r} (class_name={info.class_name!r})"
        )
    return _SEL_VARIANTS[variant]


def check_sel_handler(idx: int, var: str, info: TypeInfo, label: str) -> list[str]:
    _, create, _ = _sel_variant(info)
    return [
        f"        luaL_checktype(L, {idx}, LUA_TFUNCTION);\n",
        f"        auto {var}_handler = luax::{create}(L, {idx});\n",
        f'        if (!{var}_handler) luaL_error(L, "{label}: failed to create selector handler");\n',
    ]


def _sel_selector_expr(info: TypeInfo) -> str:
    _, _, selector = _sel_variant(info)
    return selector


def sel_selector_call_arg(info: TypeInfo) -> str:
    return _sel_selector_expr(info)


def sel_call_args(var: str, info: TypeInfo, *, handler_first: bool = True) -> list[str]:
    handler = f"{var}_handler"
    selector = _sel_selector_expr(info)
    if handler_first:
        return [handler, selector]
    return [selector, handler]


def sel_menu_call_args(var: str) -> list[str]:
    menu = TypeInfo("sel", "SEL_MenuHandler", "(sender: CCObject) -> ()", class_name="menu")
    return sel_call_args(var, menu, handler_first=True)


def check_arg(arg: Arg, info: TypeInfo, idx: int, var: str, label: str) -> list[str]:
    if info.kind == "callback":
        return _emit_callback_lambda(idx, var, info, label)
    if info.kind == "sel":
        return check_sel_handler(idx, var, info, label)
    if info.kind == "delegate":
        spec = _delegate_specs.lookup_delegate(info.cxx_type)
        if spec is None:
            raise ValueError(f"unknown delegate: {info.cxx_type}")
        return [
            f'        if (!lua_istable(L, {idx})) luaL_error(L, "{label} expected delegate table at arg %d", {idx});\n',
            f"        auto {var}_trampoline = luax::{spec.create_fn}(L, {idx});\n",
            f'        if (!{var}_trampoline) luaL_error(L, "{label}: failed to create delegate");\n',
            f"        auto {var} = static_cast<{spec.cxx_type}*>({var}_trampoline);\n",
        ]
    return emit_stack_check(info, idx, var, label, declare=True)


def push_return(
    info: TypeInfo,
    expr: str,
    owned: bool,
    *,
    owner_expr: str | None = None,
    vector_owned: bool = False,
) -> list[str]:
    if info.kind == "void":
        return ["        return 0;\n"]
    lines = _push_impl(info, expr, owned, owner_expr=owner_expr, vector_owned=vector_owned)
    return lines + ["        return 1;\n"]


def push_value(
    info: TypeInfo,
    expr: str,
    owned: bool = False,
    *,
    owner_expr: str | None = None,
    vector_owned: bool = False,
) -> list[str]:
    if info.kind == "void":
        return ["        lua_pushnil(L);\n"]
    return _push_impl(info, expr, owned, owner_expr=owner_expr, vector_owned=vector_owned)
