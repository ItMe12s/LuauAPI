#include "bindings/geode/JsonConvert.hpp"

#include "framework/stack/Stack.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace luax {
    namespace {
        bool isLuaArray(lua_State* L, int idx, int& outLen) {
            outLen = lua_objlen(L, idx);
            return outLen > 0;
        }
    } // namespace

    geode::Result<void> pushJson(lua_State* L, matjson::Value const& value, int depth) {
        if (depth > kMaxJsonDepth) {
            return geode::Err(std::string(kJsonDepthExceededMsg));
        }
        switch (value.type()) {
            case matjson::Type::Bool: lua_pushboolean(L, value.asBool().unwrapOr(false)); break;
            case matjson::Type::Number: lua_pushnumber(L, value.asDouble().unwrapOr(0.0)); break;
            case matjson::Type::String: {
                auto str = value.asString();
                if (str.isOk()) {
                    luax::push(L, str.unwrap());
                }
                else {
                    lua_pushnil(L);
                }
                break;
            }
            case matjson::Type::Array: {
                lua_newtable(L);
                int index = 1;
                for (auto const& item : value) {
                    if (auto pushed = pushJson(L, item, depth + 1); pushed.isErr()) {
                        return pushed;
                    }
                    lua_rawseti(L, -2, index++);
                }
                break;
            }
            case matjson::Type::Object: {
                lua_newtable(L);
                for (auto const& item : value) {
                    auto key = item.getKey();
                    if (!key) continue;
                    luax::push(L, *key);
                    if (auto pushed = pushJson(L, item, depth + 1); pushed.isErr()) {
                        return pushed;
                    }
                    lua_rawset(L, -3);
                }
                break;
            }
            case matjson::Type::Null:
            default: lua_pushnil(L); break;
        }
        return geode::Ok();
    }

    geode::Result<matjson::Value> toJson(lua_State* L, int idx, int depth) {
        idx = lua_absindex(L, idx);
        if (depth > kMaxJsonDepth) {
            return geode::Err(std::string(kJsonDepthExceededMsg));
        }
        switch (lua_type(L, idx)) {
            case LUA_TBOOLEAN: return geode::Ok(matjson::Value(lua_toboolean(L, idx) != 0));
            case LUA_TNUMBER:
                return geode::Ok(matjson::Value(static_cast<double>(lua_tonumber(L, idx))));
            case LUA_TSTRING: {
                std::size_t len = 0;
                char const* s = lua_tolstring(L, idx, &len);
                return geode::Ok(matjson::Value(std::string_view(s, len)));
            }
            case LUA_TTABLE: {
                int len = 0;
                if (isLuaArray(L, idx, len)) {
                    std::vector<matjson::Value> arr;
                    arr.reserve(static_cast<std::size_t>(len));
                    for (int i = 1; i <= len; ++i) {
                        lua_rawgeti(L, idx, i);
                        auto itemResult = toJson(L, -1, depth + 1);
                        if (itemResult.isErr()) {
                            return geode::Err(std::string(itemResult.unwrapErr()));
                        }
                        arr.push_back(std::move(itemResult.unwrap()));
                        lua_pop(L, 1);
                    }
                    return geode::Ok(matjson::Value(std::move(arr)));
                }
                auto obj = matjson::Value::object();
                lua_pushnil(L);
                while (lua_next(L, idx) != 0) {
                    if (lua_type(L, -2) == LUA_TSTRING) {
                        std::size_t klen = 0;
                        char const* k = lua_tolstring(L, -2, &klen);
                        auto itemResult = toJson(L, -1, depth + 1);
                        if (itemResult.isErr()) {
                            return geode::Err(std::string(itemResult.unwrapErr()));
                        }
                        obj.set(std::string_view(k, klen), std::move(itemResult.unwrap()));
                    }
                    lua_pop(L, 1);
                }
                return geode::Ok(std::move(obj));
            }
            default: return geode::Ok(matjson::Value(nullptr));
        }
    }
} // namespace luax
