from __future__ import annotations

import re
from typing import Dict

from broma_parser import Class, Method
from filtering import direct_callable, platform_value
from marshalling import push_value
from model import cxx_name, lua_namespace
from type_map import TypeInfo, classify_arg, classify_return, normalize_type


def _id(value: str) -> str:
    return re.sub(r"[^A-Za-z0-9_]", "_", value)


def hook_id(cls: Class, m: Method) -> str:
    return f"{lua_namespace(cls)}.{cls.name}:{m.name}/{len(m.args)}"


def hook_suffix(cls: Class, m: Method) -> str:
    return f"{_id(cls.name)}_{_id(m.name)}_{len(m.args)}"


def hook_offset(m: Method, target_platform: str) -> str:
    value = platform_value(m, target_platform)
    token = value.split()[0] if value else ""
    if token.startswith("0x"):
        return token
    return ""


def hookable(
    cls: Class, m: Method, objects: Dict[str, Class], target_platform: str
) -> bool:
    if m.is_static or m.is_ctor or m.is_dtor:
        return False
    if not direct_callable(cls, m, target_platform):
        return False
    if not hook_offset(m, target_platform):
        return False
    ret = classify_return(m.ret, objects)
    if ret is None:
        return False
    if "&" in normalize_type(m.ret):
        return False
    if ret.kind == "string" and ret.cxx_type.endswith("*"):
        return False
    if any("&" in normalize_type(arg.type) for arg in m.args):
        return False
    return all(classify_arg(arg.type, objects) is not None for arg in m.args)


def emit_return_override(
    info: TypeInfo, var: str, idx: str, target_id: str
) -> list[str]:
    if info.kind == "void":
        return ["        return true;\n"]
    if info.kind == "bool":
        return [
            f"        if (!lua_isboolean(L, {idx})) return false;\n",
            f"        {var} = lua_toboolean(L, {idx}) != 0;\n",
            "        return true;\n",
        ]
    if info.kind == "number":
        return [
            f"        if (!lua_isnumber(L, {idx})) return false;\n",
            f"        {var} = static_cast<{info.cxx_type}>(lua_tonumber(L, {idx}));\n",
            "        return true;\n",
        ]
    if info.kind == "string":
        if info.cxx_type == "gd::string":
            return [
                f"        if (!lua_isstring(L, {idx})) return false;\n",
                "        size_t len = 0;\n",
                f"        char const* text = lua_tolstring(L, {idx}, &len);\n",
                '        auto storage = std::string(text ? text : "", len);\n',
                "        result = gd::string(storage.c_str());\n",
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
                '        auto origin = luax::toPoint(L, -1, "hook return");\n',
                "        lua_pop(L, 1);\n",
                f'        lua_getfield(L, {idx}, "size");\n',
                "        if (!lua_istable(L, -1)) { lua_pop(L, 1); return false; }\n",
                '        auto size = luax::toSize(L, -1, "hook return");\n',
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
    if info.kind == "object":
        return [
            f"        if (lua_isnil(L, {idx})) {{ {var} = nullptr; return true; }}\n",
            f"        auto obj = luax::Usertype<{info.cxx_type[:-1]}>::tryCheck(L, {idx});\n",
            "        if (!obj) return false;\n",
            f"        {var} = obj;\n",
            "        return true;\n",
        ]
    raise ValueError(f"unsupported type kind: {info.kind}")


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
    if ret.kind == "void":
        out.append(f"        {call};\n")
    else:
        out.append(f"        auto result = {call};\n")
    if ret.kind == "bool":
        out.append("        if (!result) return false;\n")
    nargs = 1 + len(args) + (1 if ret.kind not in ("void", "bool") else 0)
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
    if ret.kind not in ("void", "bool"):
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
    offset = hook_offset(m, target_platform)
    out.append(
        f"    geode::Result<geode::Hook*> luaapi_create_hook_{suffix}(std::string const& displayName) {{\n"
    )
    out.append(
        f"        return geode::Mod::get()->hook(reinterpret_cast<void*>(geode::base::get() + {offset}), "
        f"&luaapi_hook_{suffix}, displayName, tulip::hook::TulipConvention::Default);\n"
    )
    out.append("    }\n\n")
    return "".join(out)


def emit_hook_support() -> str:
    return """    std::unordered_map<std::string, LuaHookState>& hookStates() {
        static auto* value = new std::unordered_map<std::string, LuaHookState>();
        return *value;
    }

    std::size_t& nextHookCallbackId() {
        static auto value = std::size_t(1);
        return value;
    }

    void clearHookRegistry() {
        hookStates().clear();
    }

    LuaHookState* findHookState(std::size_t callbackId) {
        for (auto& [_, state] : hookStates()) {
            for (auto const& callback : state.callbacks) {
                if (callback && callback->id == callbackId) {
                    return &state;
                }
            }
        }
        return nullptr;
    }

    std::shared_ptr<LuaHookCallback> findHookCallback(std::size_t callbackId) {
        for (auto& [_, state] : hookStates()) {
            for (auto const& callback : state.callbacks) {
                if (callback && callback->id == callbackId) {
                    return callback;
                }
            }
        }
        return nullptr;
    }

    bool stateHasLiveCallbacks(LuaHookState const& state) {
        for (auto const& callback : state.callbacks) {
            if (callback && !callback->removed) return true;
        }
        return false;
    }

    void compactRemovedCallbacks(LuaHookState& state) {
        std::size_t out = 0;
        for (auto const& callback : state.callbacks) {
            if (callback && !callback->removed) {
                state.callbacks[out++] = callback;
            }
        }
        state.callbacks.resize(out);
    }

    std::size_t compactAndCountLiveCallbacks() {
        std::size_t total = 0;
        for (auto& [_, state] : hookStates()) {
            compactRemovedCallbacks(state);
            total += state.callbacks.size();
        }
        return total;
    }

    std::size_t hookHandleId(lua_State* L) {
        return static_cast<std::size_t>(lua_tointeger(L, lua_upvalueindex(1)));
    }

    int luaapi_hook_handle_enable(lua_State* L) {
        auto* state = findHookState(hookHandleId(L));
        if (!state || !state->hook) {
            lua_pushboolean(L, false);
            return 1;
        }
        auto result = state->hook->enable();
        lua_pushboolean(L, result.isOk());
        if (result.isErr()) {
            luax::push(L, result.unwrapErr());
            return 2;
        }
        return 1;
    }

    int luaapi_hook_handle_disable(lua_State* L) {
        auto* state = findHookState(hookHandleId(L));
        if (!state || !state->hook) {
            lua_pushboolean(L, false);
            return 1;
        }
        auto result = state->hook->disable();
        lua_pushboolean(L, result.isOk());
        if (result.isErr()) {
            luax::push(L, result.unwrapErr());
            return 2;
        }
        return 1;
    }

    int luaapi_hook_handle_remove(lua_State* L) {
        auto id = hookHandleId(L);
        auto callback = findHookCallback(id);
        auto* state = findHookState(id);
        if (!callback || callback->removed) {
            lua_pushboolean(L, false);
            return 1;
        }
        callback->removed = true;
        callback->ref.reset();
        if (state && state->hook && !stateHasLiveCallbacks(*state)) {
            auto result = state->hook->disable();
            if (result.isErr()) {
                geode::log::warn("luau hook disable failed after remove: {}", result.unwrapErr());
            }
        }
        lua_pushboolean(L, true);
        return 1;
    }

    int luaapi_hook_handle_is_enabled(lua_State* L) {
        auto* state = findHookState(hookHandleId(L));
        lua_pushboolean(L, state && state->hook && state->hook->isEnabled());
        return 1;
    }

    void pushHookHandle(lua_State* L, std::size_t callbackId) {
        lua_createtable(L, 0, 4);
        lua_pushinteger(L, static_cast<lua_Integer>(callbackId));
        lua_pushcclosure(L, &luaapi_hook_handle_enable, nullptr, 1);
        lua_setfield(L, -2, "enable");
        lua_pushinteger(L, static_cast<lua_Integer>(callbackId));
        lua_pushcclosure(L, &luaapi_hook_handle_disable, nullptr, 1);
        lua_setfield(L, -2, "disable");
        lua_pushinteger(L, static_cast<lua_Integer>(callbackId));
        lua_pushcclosure(L, &luaapi_hook_handle_remove, nullptr, 1);
        lua_setfield(L, -2, "remove");
        lua_pushinteger(L, static_cast<lua_Integer>(callbackId));
        lua_pushcclosure(L, &luaapi_hook_handle_is_enabled, nullptr, 1);
        lua_setfield(L, -2, "isEnabled");
    }

    int luaapi_geode_hook(lua_State* L) {
        if (lua_gettop(L) != 2) luaL_error(L, "geode.hook expected 2 args");
        char const* id = lua_tostring(L, 1);
        if (!id || !*id) luaL_error(L, "geode.hook expected target id at arg 1");
        if (!lua_isfunction(L, 2)) luaL_error(L, "geode.hook expected function at arg 2");

        auto* target = findHookTarget(id);
        if (!target) luaL_error(L, "geode.hook target not found: %s", id);

        auto& state = hookStates()[id];
        state.target = target;
        auto liveTotal = compactAndCountLiveCallbacks();
        if (liveTotal >= luax::kMaxHookCallbacksGlobal) {
            luaL_error(L, "geode.hook global callback limit exceeded");
        }
        if (state.callbacks.size() >= luax::kMaxHookCallbacksPerTarget) {
            luaL_error(L, "geode.hook target callback limit exceeded for %s", id);
        }

        if (!state.hook) {
            auto result = target->createHook(target->displayName);
            if (result.isErr()) {
                luaL_error(L, "geode.hook failed for %s: %s", id, result.unwrapErr().c_str());
            }
            state.hook = result.unwrap();
            if (!state.hook->isEnabled()) {
                auto enableResult = state.hook->enable();
                if (enableResult.isErr()) {
                    luaL_error(L, "geode.hook enable failed for %s: %s", id, enableResult.unwrapErr().c_str());
                }
            }
        } else if (!state.hook->isEnabled()) {
            auto result = state.hook->enable();
            if (result.isErr()) {
                luaL_error(L, "geode.hook enable failed for %s: %s", id, result.unwrapErr().c_str());
            }
        }

        auto callback = std::make_shared<LuaHookCallback>();
        callback->id = nextHookCallbackId()++;
        callback->ref.reset(L, 2);
        state.callbacks.push_back(callback);
        pushHookHandle(L, callback->id);
        return 1;
    }

"""
