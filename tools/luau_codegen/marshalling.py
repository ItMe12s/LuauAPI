from __future__ import annotations

from broma_parser import Arg
from type_map import TypeInfo


def _push_impl(info: TypeInfo, expr: str, owned: bool) -> list[str]:
    if info.kind == "bool":
        return [f"        luax::push(L, {expr});\n"]
    if info.kind == "number":
        return [f"        lua_pushnumber(L, static_cast<double>({expr}));\n"]
    if info.kind == "enum":
        return [
            f"        lua_pushnumber(L, static_cast<double>(static_cast<int>({expr})));\n"
        ]
    if info.kind == "string":
        if info.cxx_type.endswith("*"):
            return [f"        luax::push(L, {expr});\n"]
        return [f"        luax::push(L, std::string({expr}));\n"]
    if info.kind == "value":
        if info.lua_type == "CCPoint":
            return [f"        luax::pushPoint(L, {expr});\n"]
        if info.lua_type == "CCSize":
            return [f"        luax::pushSize(L, {expr});\n"]
        if info.lua_type == "CCRect":
            return [f"        luax::pushRect(L, {expr});\n"]
        if info.lua_type == "RGBColor":
            return [f"        luax::pushColor3B(L, {expr});\n"]
        if info.lua_type == "UIButtonConfig":
            return [f"        luax::pushUIButtonConfig(L, {expr});\n"]
    if info.kind == "object":
        push = "pushOwned" if owned else "pushBorrowed"
        return [f"        luax::Usertype<{info.cxx_type[:-1]}>::{push}(L, {expr});\n"]
    raise ValueError(f"unsupported type kind: {info.kind}")


def check_arg(arg: Arg, info: TypeInfo, idx: int, var: str, label: str) -> list[str]:
    cxx = info.cxx_type
    if info.kind == "bool":
        return [f'        auto {var} = luax::check<bool>(L, {idx}, "{label}");\n']
    if info.kind in ("number", "enum"):
        if cxx in ("float", "double"):
            return [f'        auto {var} = luax::check<{cxx}>(L, {idx}, "{label}");\n']
        return [
            f'        auto {var} = static_cast<{cxx}>(luax::check<int>(L, {idx}, "{label}"));\n'
        ]
    if info.kind == "string":
        if cxx == "char const*" or cxx == "const char*":
            return [
                f'        auto {var}_storage = luax::check<std::string>(L, {idx}, "{label}");\n',
                f"        auto {var} = {var}_storage.c_str();\n",
            ]
        if cxx == "gd::string":
            return [
                f'        auto {var}_storage = luax::check<std::string>(L, {idx}, "{label}");\n',
                f"        auto {var} = gd::string({var}_storage.c_str());\n",
            ]
        return [
            f'        auto {var} = luax::check<std::string>(L, {idx}, "{label}");\n'
        ]
    if info.kind == "value":
        if info.lua_type == "CCPoint":
            return [f'        auto {var} = luax::toPoint(L, {idx}, "{label}");\n']
        if info.lua_type == "CCSize":
            return [f'        auto {var} = luax::toSize(L, {idx}, "{label}");\n']
        if info.lua_type == "CCRect":
            return [f'        auto {var} = luax::toRect(L, {idx}, "{label}");\n']
        if info.lua_type == "RGBColor":
            return [f'        auto {var} = luax::toColor3B(L, {idx}, "{label}");\n']
        if info.lua_type == "UIButtonConfig":
            return [
                f'        auto {var} = luax::toUIButtonConfig(L, {idx}, "{label}");\n'
            ]
    if info.kind == "object":
        return [
            f'        auto {var} = luax::Usertype<{info.cxx_type[:-1]}>::check(L, {idx}, "{label}");\n'
        ]
    raise ValueError(f"unsupported type kind: {info.kind}")


def push_return(info: TypeInfo, expr: str, owned: bool) -> list[str]:
    if info.kind == "void":
        return ["        return 0;\n"]
    lines = _push_impl(info, expr, owned)
    return lines + ["        return 1;\n"]


def push_value(info: TypeInfo, expr: str, owned: bool = False) -> list[str]:
    if info.kind == "void":
        return ["        lua_pushnil(L);\n"]
    return _push_impl(info, expr, owned)
