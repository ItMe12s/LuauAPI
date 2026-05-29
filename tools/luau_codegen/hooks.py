from __future__ import annotations

import re
from typing import Dict

from android_symbol import android_symbol
from broma_parser import Class, Method
from filtering import direct_callable, platform_value
from link_attrs import class_link_platforms, platform_aliases
from marshalling import push_value
from model import cxx_name, lua_namespace
from type_map import (
    TypeInfo,
    classify_arg,
    classify_return,
    is_out_reference,
    normalize_type,
)


def _id(value: str) -> str:
    return re.sub(r"[^A-Za-z0-9_]", "_", value)


def _cstr(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


def hook_id(cls: Class, m: Method) -> str:
    return f"{lua_namespace(cls)}.{cls.name}:{m.name}/{len(m.args)}"


def hook_suffix(cls: Class, m: Method) -> str:
    return f"{_id(cls.name)}_{_id(m.name)}_{len(m.args)}"


def concrete_hook_platform(target_platform: str) -> bool:
    if target_platform == "android":
        return False
    return True


def _class_link_platforms(cls: Class) -> set[str]:
    return class_link_platforms(cls)


def _android_linked(cls: Class, target_platform: str) -> bool:
    if target_platform not in ("android32", "android64"):
        return False
    links = _class_link_platforms(cls)
    return bool(links & platform_aliases(target_platform))


def _apple_linked(cls: Class, target_platform: str) -> bool:
    if target_platform not in ("ios", "mac", "imac", "m1"):
        return False
    links = _class_link_platforms(cls)
    return bool(links & platform_aliases(target_platform))


def hook_offset(m: Method, target_platform: str) -> str:
    if not concrete_hook_platform(target_platform):
        return ""
    value = platform_value(m, target_platform)
    token = value.split()[0] if value else ""
    if re.fullmatch(r"0x[0-9A-Fa-f]+", token):
        return token
    return ""


def hook_address_expr(cls: Class, m: Method, target_platform: str) -> str:
    offset = hook_offset(m, target_platform)
    if offset:
        return f"reinterpret_cast<void*>(geode::base::get() + {offset})"
    if not concrete_hook_platform(target_platform):
        return ""
    if _android_linked(cls, target_platform):
        symbol = android_symbol(cls, m)
        return f'dlsym(luaapi_android_libcocos(), "{symbol}")'
    if _apple_linked(cls, target_platform):
        symbol = android_symbol(cls, m)
        return f'dlsym(RTLD_DEFAULT, "{symbol}")'
    return ""


def hookable(
    cls: Class, m: Method, objects: Dict[str, Class], target_platform: str
) -> bool:
    if m.is_static or m.is_ctor or m.is_dtor:
        return False
    if not direct_callable(cls, m, target_platform):
        return False
    if not hook_address_expr(cls, m, target_platform):
        return False
    ret = classify_return(m.ret, objects)
    if ret is None:
        return False
    if "&" in normalize_type(m.ret):
        return False
    if ret.kind == "string" and ret.cxx_type.endswith("*"):
        return False
    if any(is_out_reference(arg.type) for arg in m.args):
        return False
    return all(classify_arg(arg.type, objects) is not None for arg in m.args)


def emit_value_decode(info: TypeInfo, var: str, idx: str, context: str) -> list[str]:
    if info.kind == "void":
        return ["        return true;\n"]
    if info.kind == "bool":
        return [
            f"        if (!lua_isboolean(L, {idx})) return false;\n",
            f"        {var} = lua_toboolean(L, {idx}) != 0;\n",
            "        return true;\n",
        ]
    if info.kind in ("number", "enum"):
        return [
            f"        if (!lua_isnumber(L, {idx})) return false;\n",
            f"        {var} = static_cast<{info.cxx_type}>(lua_tonumber(L, {idx}));\n",
            "        return true;\n",
        ]
    if info.kind == "wideint":
        return [
            f"        return luax::tryIntegerString<{info.cxx_type}>(L, {idx}, &{var});\n"
        ]
    if info.kind == "string":
        if info.cxx_type == "gd::string":
            return [
                f"        if (!lua_isstring(L, {idx})) return false;\n",
                "        size_t len = 0;\n",
                f"        char const* text = lua_tolstring(L, {idx}, &len);\n",
                '        auto storage = std::string(text ? text : "", len);\n',
                f"        {var} = gd::string(storage.c_str());\n",
                "        return true;\n",
            ]
        return [
            f"        if (!lua_isstring(L, {idx})) return false;\n",
            "        size_t len = 0;\n",
            f"        char const* text = lua_tolstring(L, {idx}, &len);\n",
            f'        {var} = std::string(text ? text : "", len);\n',
            "        return true;\n",
        ]
    if info.kind == "value":
        if info.lua_type == "CCPoint":
            return [
                f"        if (!lua_istable(L, {idx})) return false;\n",
                f'        lua_getfield(L, {idx}, "x");\n',
                "        if (!lua_isnumber(L, -1)) { lua_pop(L, 1); return false; }\n",
                "        auto x = static_cast<float>(lua_tonumber(L, -1));\n",
                "        lua_pop(L, 1);\n",
                f'        lua_getfield(L, {idx}, "y");\n',
                "        if (!lua_isnumber(L, -1)) { lua_pop(L, 1); return false; }\n",
                "        auto y = static_cast<float>(lua_tonumber(L, -1));\n",
                "        lua_pop(L, 1);\n",
                f"        {var} = cocos2d::CCPoint(x, y);\n",
                "        return true;\n",
            ]
        if info.lua_type == "CCSize":
            return [
                f"        if (!lua_istable(L, {idx})) return false;\n",
                f'        lua_getfield(L, {idx}, "width");\n',
                "        if (!lua_isnumber(L, -1)) { lua_pop(L, 1); return false; }\n",
                "        auto width = static_cast<float>(lua_tonumber(L, -1));\n",
                "        lua_pop(L, 1);\n",
                f'        lua_getfield(L, {idx}, "height");\n',
                "        if (!lua_isnumber(L, -1)) { lua_pop(L, 1); return false; }\n",
                "        auto height = static_cast<float>(lua_tonumber(L, -1));\n",
                "        lua_pop(L, 1);\n",
                f"        {var} = cocos2d::CCSize(width, height);\n",
                "        return true;\n",
            ]
        if info.lua_type == "CCRect":
            return [
                f"        if (!lua_istable(L, {idx})) return false;\n",
                f'        lua_getfield(L, {idx}, "origin");\n',
                "        if (!lua_istable(L, -1)) { lua_pop(L, 1); return false; }\n",
                f'        auto origin = luax::toPoint(L, -1, "{context}");\n',
                "        lua_pop(L, 1);\n",
                f'        lua_getfield(L, {idx}, "size");\n',
                "        if (!lua_istable(L, -1)) { lua_pop(L, 1); return false; }\n",
                f'        auto size = luax::toSize(L, -1, "{context}");\n',
                "        lua_pop(L, 1);\n",
                f"        {var} = cocos2d::CCRect(origin.x, origin.y, size.width, size.height);\n",
                "        return true;\n",
            ]
        if info.lua_type == "RGBColor":
            return [
                f"        if (!lua_istable(L, {idx})) return false;\n",
                f'        lua_getfield(L, {idx}, "r");\n',
                "        if (!lua_isnumber(L, -1)) { lua_pop(L, 1); return false; }\n",
                "        auto r = static_cast<GLubyte>(lua_tointeger(L, -1));\n",
                "        lua_pop(L, 1);\n",
                f'        lua_getfield(L, {idx}, "g");\n',
                "        if (!lua_isnumber(L, -1)) { lua_pop(L, 1); return false; }\n",
                "        auto g = static_cast<GLubyte>(lua_tointeger(L, -1));\n",
                "        lua_pop(L, 1);\n",
                f'        lua_getfield(L, {idx}, "b");\n',
                "        if (!lua_isnumber(L, -1)) { lua_pop(L, 1); return false; }\n",
                "        auto b = static_cast<GLubyte>(lua_tointeger(L, -1));\n",
                "        lua_pop(L, 1);\n",
                f"        {var} = cocos2d::ccColor3B{{r, g, b}};\n",
                "        return true;\n",
            ]
        if info.lua_type == "UIButtonConfig":
            return [
                f"        if (!lua_istable(L, {idx})) return false;\n",
                f'        {var} = luax::toUIButtonConfig(L, {idx}, "{context}");\n',
                "        return true;\n",
            ]
    if info.kind == "object":
        return [
            f"        if (lua_isnil(L, {idx})) {{ {var} = nullptr; return true; }}\n",
            f"        auto obj = luax::Usertype<{info.cxx_type[:-1]}>::tryCheck(L, {idx});\n",
            "        if (!obj) return false;\n",
            f"        {var} = obj;\n",
            "        return true;\n",
        ]
    raise ValueError(f"unsupported type kind: {info.kind}")


def emit_return_override(
    info: TypeInfo, var: str, idx: str, target_id: str
) -> list[str]:
    return emit_value_decode(info, var, idx, "hook return")


def emit_hook_target(
    cls: Class, m: Method, objects: Dict[str, Class], target_platform: str
) -> str:
    ret = classify_return(m.ret, objects)
    assert ret is not None
    args = []
    call_args = []
    for i, arg in enumerate(m.args):
        info = classify_arg(arg.type, objects)
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
    out = [f"    {ret_type} luaapi_hook_{suffix}({params_text}) {{\n"]
    if ret.kind != "void":
        out.append(f"        {ret.cxx_type} result{{}};\n")
    for _, info, name in args:
        if info.kind == "string" and info.cxx_type.endswith("*"):
            out.append(f"        std::string {name}Storage;\n")
    out.append("        bool skipOriginal = false;\n")
    out.append(
        f'        skipOriginal = luauapi_gen::runLuaPreHooks("{target_id}", {1 + len(args)}, [&](lua_State* L) {{\n'
    )
    out.extend(
        f"    {line}"
        for line in push_value(
            TypeInfo("object", f"{cxx_cls}*", cls.name, cls.name), "self"
        )
    )
    for _, info, name in args:
        out.extend(f"    {line}" for line in push_value(info, name))
    out.append("        }, [&](lua_State* L, int idx) -> bool {\n")
    if args:
        arg_names = [arg.name for arg, _, _ in args]
        named_args = all(arg_names) and len(set(arg_names)) == len(arg_names)
        out.append("            if (!lua_istable(L, idx)) return false;\n")
        out.append(
            f"            bool useArrayArgs = lua_objlen(L, idx) == {len(args)};\n"
        )
        out.append(
            f"            bool useNamedArgs = {'true' if named_args else 'false'};\n"
        )
        out.append("            if (!useArrayArgs && !useNamedArgs) return false;\n")
        for _, info, name in args:
            if info.kind == "string" and info.cxx_type.endswith("*"):
                out.append(f"            std::string {name}StorageOverride;\n")
            else:
                out.append(f"            {info.cxx_type} {name}Override{{}};\n")
        for i, (arg, info, name) in enumerate(args, start=1):
            if info.kind == "string" and info.cxx_type.endswith("*"):
                out.append(
                    f'            if (useArrayArgs) lua_rawgeti(L, idx, {i}); else lua_getfield(L, idx, "{_cstr(arg.name)}");\n'
                )
                out.append(
                    "            if (!lua_isstring(L, -1)) { lua_pop(L, 1); return false; }\n"
                )
                out.append(f"            size_t {name}Len = 0;\n")
                out.append(
                    f"            char const* {name}Text = lua_tolstring(L, -1, &{name}Len);\n"
                )
                out.append(
                    f'            {name}StorageOverride = std::string({name}Text ? {name}Text : "", {name}Len);\n'
                )
                out.append("            lua_pop(L, 1);\n")
                continue
            tmp = f"{name}Override"
            out.append(
                f'            if (useArrayArgs) lua_rawgeti(L, idx, {i}); else lua_getfield(L, idx, "{_cstr(arg.name)}");\n'
            )
            out.append(
                f"            auto decode_{name} = [&](int valueIdx) -> bool {{\n"
            )
            out.extend(
                f"        {line}"
                for line in emit_value_decode(info, tmp, "valueIdx", "hook args")
            )
            out.append("            };\n")
            out.append(
                f"            if (!decode_{name}(-1)) {{ lua_pop(L, 1); return false; }}\n"
            )
            out.append("            lua_pop(L, 1);\n")
        for _, info, name in args:
            if info.kind == "string" and info.cxx_type.endswith("*"):
                out.append(f"            {name}Storage = {name}StorageOverride;\n")
                out.append(f"            {name} = {name}Storage.c_str();\n")
            else:
                out.append(f"            {name} = {name}Override;\n")
        out.append("            return true;\n")
    else:
        out.append(
            "            return lua_istable(L, idx) && lua_objlen(L, idx) == 0;\n"
        )
    out.append("        }, [&](lua_State* L, int idx) -> bool {\n")
    if ret.kind == "void":
        out.append("            return true;\n")
    else:
        out.extend(
            f"    {line}"
            for line in emit_value_decode(ret, "result", "idx", "hook skip")
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
        for line in push_value(
            TypeInfo("object", f"{cxx_cls}*", cls.name, cls.name), "self"
        )
    )
    for _, info, name in args:
        out.extend(f"    {line}" for line in push_value(info, name))
    if ret.kind != "void":
        out.extend(f"    {line}" for line in push_value(ret, "result"))
    out.append("        }, [&](lua_State* L, int idx) -> bool {\n")
    if ret.kind == "void":
        out.append("            return true;\n")
    else:
        out.extend(
            f"    {line}"
            for line in emit_return_override(ret, "result", "idx", target_id)
        )
    out.append("        });\n")
    if ret.kind != "void":
        out.append("        return result;\n")
    out.append("    }\n\n")
    address = hook_address_expr(cls, m, target_platform)
    out.append(
        f"    geode::Result<geode::Hook*> luaapi_create_hook_{suffix}(std::string const& displayName) {{\n"
    )
    out.append(
        f"        return geode::Mod::get()->hook({address}, "
        f"&luaapi_hook_{suffix}, displayName, tulip::hook::TulipConvention::Default);\n"
    )
    out.append("    }\n\n")
    return "".join(out)
