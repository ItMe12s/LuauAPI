#include "JsonConvert.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace luax {
    namespace {
        void pushString(lua_State* L, std::string_view s) {
            lua_pushlstring(L, s.data(), s.size());
        }

        bool isLuaArray(lua_State* L, int idx, int& outLen) {
            outLen = lua_objlen(L, idx);
            return outLen > 0;
        }
    }

    void pushJson(lua_State* L, matjson::Value const& value, int depth) {
        if (depth > kMaxJsonDepth) {
            lua_pushnil(L);
            return;
        }
        switch (value.type()) {
            case matjson::Type::Bool:
                lua_pushboolean(L, value.asBool().unwrapOr(false));
                break;
            case matjson::Type::Number:
                lua_pushnumber(L, value.asDouble().unwrapOr(0.0));
                break;
            case matjson::Type::String: {
                auto str = value.asString();
                if (str.isOk()) {
                    pushString(L, str.unwrap());
                } else {
                    lua_pushnil(L);
                }
                break;
            }
            case matjson::Type::Array: {
                lua_checkstack(L, 4);
                lua_newtable(L);
                int index = 1;
                for (auto const& item : value) {
                    pushJson(L, item, depth + 1);
                    lua_rawseti(L, -2, index++);
                }
                break;
            }
            case matjson::Type::Object: {
                lua_checkstack(L, 4);
                lua_newtable(L);
                for (auto const& item : value) {
                    auto key = item.getKey();
                    if (!key) continue;
                    pushString(L, *key);
                    pushJson(L, item, depth + 1);
                    lua_rawset(L, -3);
                }
                break;
            }
            case matjson::Type::Null:
            default:
                lua_pushnil(L);
                break;
        }
    }

    matjson::Value toJson(lua_State* L, int idx, int depth) {
        idx = lua_absindex(L, idx);
        switch (lua_type(L, idx)) {
            case LUA_TBOOLEAN:
                return matjson::Value(lua_toboolean(L, idx) != 0);
            case LUA_TNUMBER:
                return matjson::Value(static_cast<double>(lua_tonumber(L, idx)));
            case LUA_TSTRING: {
                std::size_t len = 0;
                char const* s = lua_tolstring(L, idx, &len);
                return matjson::Value(std::string_view(s, len));
            }
            case LUA_TTABLE: {
                if (depth > kMaxJsonDepth) return matjson::Value(nullptr);
                int len = 0;
                if (isLuaArray(L, idx, len)) {
                    std::vector<matjson::Value> arr;
                    arr.reserve(static_cast<std::size_t>(len));
                    for (int i = 1; i <= len; ++i) {
                        lua_rawgeti(L, idx, i);
                        arr.push_back(toJson(L, -1, depth + 1));
                        lua_pop(L, 1);
                    }
                    return matjson::Value(std::move(arr));
                }
                auto obj = matjson::Value::object();
                lua_pushnil(L);
                while (lua_next(L, idx) != 0) {
                    if (lua_type(L, -2) == LUA_TSTRING) {
                        std::size_t klen = 0;
                        char const* k = lua_tolstring(L, -2, &klen);
                        obj.set(std::string_view(k, klen), toJson(L, -1, depth + 1));
                    }
                    lua_pop(L, 1);
                }
                return obj;
            }
            default:
                return matjson::Value(nullptr);
        }
    }
}
