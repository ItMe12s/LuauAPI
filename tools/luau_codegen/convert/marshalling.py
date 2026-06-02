from __future__ import annotations

from luau_codegen.parse.broma import Arg
from luau_codegen.convert.type_map import TypeInfo

_VALUE_CHECK_TYPES = {
    "CCPoint": "cocos2d::CCPoint",
    "CCSize": "cocos2d::CCSize",
    "CCRect": "cocos2d::CCRect",
    "RGBColor": "cocos2d::ccColor3B",
    "UIButtonConfig": "UIButtonConfig",
}

_UNSIGNED = {
    "unsigned",
    "unsigned int",
    "unsigned char",
    "unsigned short",
    "uint8_t",
    "uint16_t",
    "uint32_t",
}
_WIDE = {
    "long long",
    "unsigned long long",
    "int64_t",
    "uint64_t",
    "unsigned long",
    "long",
    "size_t",
    "ptrdiff_t",
}


def _prefix(declare: bool, var: str) -> str:
    return f"auto {var}" if declare else var


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
        return [
            f'        {_prefix(declare, var)} = luax::check<bool>(L, {idx}, "{label}");\n'
        ]
    if info.kind in ("number", "enum"):
        if cxx in ("float", "double"):
            return [
                f'        {_prefix(declare, var)} = luax::check<{cxx}>(L, {idx}, "{label}");\n'
            ]
        if cxx in _WIDE:
            check_type = "double"
        elif cxx in _UNSIGNED:
            check_type = "unsigned"
        else:
            check_type = "int"
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
        check_type = _VALUE_CHECK_TYPES.get(info.lua_type)
        if check_type is None:
            raise ValueError(f"unsupported value type: {info.lua_type}")
        return [
            f'        {_prefix(declare, var)} = luax::check<{check_type}>(L, {idx}, "{label}");\n'
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
        if info.element_type is None or info.element_type.kind != "object":
            raise ValueError("vector view requires object element type")
        elem = info.element_type.cxx_type[:-1]
        return [
            f'        {_prefix(declare, var)} = luax::checkObjectVectorView<{elem}>(L, {idx}, "{label}");\n'
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
    if info.kind == "number":
        return [f"{indent}lua_pushnumber(L, static_cast<double>({expr}));\n"]
    if info.kind == "wideint":
        return [f"{indent}luax::pushIntegerString(L, {expr});\n"]
    if info.kind == "enum":
        return [
            f"{indent}lua_pushnumber(L, static_cast<double>(static_cast<int>({expr})));\n"
        ]
    if info.kind == "string":
        if info.cxx_type.endswith("*"):
            return [f"{indent}luax::push(L, {expr});\n"]
        return [f"{indent}luax::push(L, std::string({expr}));\n"]
    if info.kind == "value":
        if info.lua_type in _VALUE_CHECK_TYPES:
            return [f"{indent}luax::push(L, {expr});\n"]
    if info.kind == "object":
        push = "pushOwned" if owned else "pushBorrowed"
        return [f"{indent}luax::Usertype<{info.cxx_type[:-1]}>::{push}(L, {expr});\n"]
    if info.kind == "vector_view":
        if info.element_type is None or info.element_type.kind != "object":
            raise ValueError("vector view requires object element type")
        elem = info.element_type.cxx_type[:-1]
        if vector_owned or not owner_expr:
            return [f"{indent}luax::pushOwnedReadOnlyVectorView<{elem}>(L, {expr});\n"]
        return [
            f"{indent}luax::pushReadOnlyVectorView<{elem}>(L, {expr}, {owner_expr});\n"
        ]
    raise ValueError(f"unsupported type kind: {info.kind}")


def _emit_callback_lambda(idx: int, var: str, info: TypeInfo, label: str) -> list[str]:
    params = ", ".join(
        f"{cb.cxx_type} {var}_p{i}" for i, cb in enumerate(info.callback_args)
    )
    lines = [
        f"        luaL_checktype(L, {idx}, LUA_TFUNCTION);\n",
        f"        auto {var}_cb = std::make_shared<luax::LuaCallback>(L, {idx});\n",
        f"        auto {var} = [{var}_cb]({params}) {{\n",
    ]
    if info.callback_args:
        ctx_fields = "; ".join(
            f"{cb.cxx_type} p{i}" for i, cb in enumerate(info.callback_args)
        )
        ctx_init = ", ".join(f"{var}_p{i}" for i in range(len(info.callback_args)))
        lines.append(f"            struct {var}_Ctx {{ {ctx_fields}; }};\n")
        lines.append(f"            {var}_Ctx ctx{{ {ctx_init} }};\n")
        lines.append(
            f'            {var}_cb->invoke({len(info.callback_args)}, 0, "{label} callback", luax::kHookScriptDeadlineMs,\n'
        )
        lines.append(f"                +[](lua_State* L, void* raw) {{\n")
        lines.append(f"                    auto* c = static_cast<{var}_Ctx*>(raw);\n")
        for i, cb in enumerate(info.callback_args):
            lines.extend(
                _push_impl(cb, f"c->p{i}", False, indent="                    ")
            )
        lines.append("                }, &ctx);\n")
    else:
        lines.append(
            f'            {var}_cb->invoke(0, 0, "{label} callback", luax::kHookScriptDeadlineMs);\n'
        )
    lines.append("        };\n")
    return lines


def check_sel_menu_handler(idx: int, var: str, label: str) -> list[str]:
    return [
        f"        luaL_checktype(L, {idx}, LUA_TFUNCTION);\n",
        f"        auto {var}_handler = luax::LuaMenuHandler::create(L, {idx});\n",
        f'        if (!{var}_handler) luaL_error(L, "{label}: failed to create menu handler");\n',
    ]


def sel_menu_call_args(var: str) -> list[str]:
    return [
        f"{var}_handler",
        "menu_selector(luax::LuaMenuHandler::onCallback)",
    ]


def check_arg(arg: Arg, info: TypeInfo, idx: int, var: str, label: str) -> list[str]:
    if info.kind == "callback":
        return _emit_callback_lambda(idx, var, info, label)
    if info.kind == "sel":
        return check_sel_menu_handler(idx, var, label)
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
    lines = _push_impl(
        info, expr, owned, owner_expr=owner_expr, vector_owned=vector_owned
    )
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
    return _push_impl(
        info, expr, owned, owner_expr=owner_expr, vector_owned=vector_owned
    )
