#include "lua/Config.hpp"
#include "lua/bindings/Binding.hpp"
#include "lua/bindings/framework/LuaCallback.hpp"
#include "lua/bindings/framework/Stack.hpp"
#include "lua/bindings/framework/TableUtil.hpp"

#include <Geode/utils/permission.hpp>
#include <lua.h>
#include <lualib.h>
#include <memory>

namespace {
    using namespace luax;
    namespace permission = geode::utils::permission;

    permission::Permission checkPermission(lua_State* L, int idx, char const* method) {
        int value = check<int>(L, idx, method);
        if (value != static_cast<int>(permission::Permission::ReadAllFiles) &&
            value != static_cast<int>(permission::Permission::RecordAudio)) {
            luaL_error(
                L, "%s: invalid permission %d (use geode.utils.permission.Permission.*)", method, value
            );
        }
        return static_cast<permission::Permission>(value);
    }

    int permGetStatus(lua_State* L) {
        auto perm = checkPermission(L, 1, "geode.utils.permission.getPermissionStatus");
        push(L, permission::getPermissionStatus(perm));
        return 1;
    }

    int permRequest(lua_State* L) {
        auto perm = checkPermission(L, 1, "geode.utils.permission.requestPermission");
        luaL_checktype(L, 2, LUA_TFUNCTION);
        auto cb = std::make_shared<luax::LuaCallback>(L, 2);
        permission::requestPermission(perm, [cb](bool granted) {
            bool g = granted;
            cb->invoke(
                1,
                0,
                "geode.utils.permission.requestPermission",
                kHookScriptDeadlineMs,
                +[](lua_State* L, void* raw) {
                    lua_pushboolean(L, *static_cast<bool*>(raw));
                },
                &g
            );
        });
        return 0;
    }
} // namespace

namespace luax {
    geode::Result<void> registerGeodePermission(lua_State* L) {
        getOrCreateTable(L, "geode.utils.permission");
        setTableCFunction(L, -1, "getPermissionStatus", &permGetStatus);
        setTableCFunction(L, -1, "requestPermission", &permRequest);

        lua_createtable(L, 0, 2);
        setIntField(L, "ReadAllFiles", static_cast<int>(permission::Permission::ReadAllFiles));
        setIntField(L, "RecordAudio", static_cast<int>(permission::Permission::RecordAudio));
        lua_setfield(L, -2, "Permission");

        lua_pop(L, 1);
        return geode::Ok();
    }
} // namespace luax

#if !defined(LUAUAPI_HOST_TESTS)
LUAX_BINDING(geode_permission_lib, registerGeodePermission)
#endif
