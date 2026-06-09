#include "lua/bindings/geode/ModSandbox.hpp"

#include "lua/bindings/framework/Stack.hpp"
#include "lua/bindings/geode/CurrentMod.hpp"
#include "lua/module/PathSandbox.hpp"

#include <Geode/loader/Mod.hpp>
#include <lua.h>
#include <lualib.h>
#include <string>

namespace {
    bool rootDir(lua_State* L, std::string const& name, std::filesystem::path& out, bool& writable) {
        auto* mod = luax::requireCurrentMod(L);
        if (name == "save") {
            out = mod->getSaveDir();
            writable = true;
            return true;
        }
        if (name == "config") {
            out = mod->getConfigDir();
            writable = true;
            return true;
        }
        if (name == "persistent") {
            out = mod->getPersistentDir();
            writable = true;
            return true;
        }
        if (name == "resources") {
            out = mod->getResourcesDir();
            writable = false;
            return true;
        }
        return false;
    }
} // namespace

namespace luax {
    std::optional<SandboxTarget> resolveSandboxTarget(
        lua_State* L, int rootIdx, int pathIdx, char const* method, bool requireWritable
    ) {
        auto root = check<std::string>(L, rootIdx, method);
        auto rel = check<std::string>(L, pathIdx, method);

        std::filesystem::path dir;
        bool writable = false;
        if (!rootDir(L, root, dir, writable)) {
            luaL_error(
                L,
                "%s: unknown root '%s' (expected save/config/persistent/resources)",
                method,
                root.c_str()
            );
        }

        if (requireWritable && !writable) {
            pushNilErr(L, "root is read-only");
            return std::nullopt;
        }

        auto resolved = resolveInsideRoot(dir, rel);
        if (resolved.isErr()) {
            pushNilErr(L, resolved.unwrapErr());
            return std::nullopt;
        }
        return SandboxTarget{resolved.unwrap(), writable};
    }
} // namespace luax
