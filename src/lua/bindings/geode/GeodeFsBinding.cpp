#include "lua/Config.hpp"
#include "lua/bindings/Binding.hpp"
#include "lua/bindings/framework/Stack.hpp"
#include "lua/bindings/framework/TableUtil.hpp"
#include "lua/bindings/geode/CurrentMod.hpp"
#include "lua/module/PathSandbox.hpp"

#include <Geode/loader/Mod.hpp>
#include <Geode/utils/file.hpp>
#include <Geode/utils/string.hpp>

#include <lua.h>
#include <lualib.h>

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace {
    using namespace luax;

    bool rootDir(lua_State* L, std::string const& name, std::filesystem::path& out, bool& writable) {
        auto* mod = requireCurrentMod(L);
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

    struct Target {
        std::filesystem::path path;
        bool writable;
    };

    std::optional<Target> resolveTarget(lua_State* L, char const* method) {
        auto root = check<std::string>(L, 1, method);
        auto rel = check<std::string>(L, 2, method);

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

        auto resolved = resolveInsideRoot(dir, rel);
        if (resolved.isErr()) {
            lua_pushnil(L);
            push(L, std::string(resolved.unwrapErr()));
            return std::nullopt;
        }
        return Target{resolved.unwrap(), writable};
    }

    int pushReadOnlyError(lua_State* L) {
        lua_pushnil(L);
        push(L, std::string("root is read-only"));
        return 2;
    }

    int fsRead(lua_State* L) {
        auto target = resolveTarget(L, "geode.fs.read");
        if (!target) return 2;

        auto contents = geode::utils::file::readString(target->path);
        if (contents.isErr()) {
            lua_pushnil(L);
            push(L, std::string(contents.unwrapErr()));
            return 2;
        }
        auto data = std::move(contents.unwrap());
        if (data.size() > kMaxFsReadBytes) {
            lua_pushnil(L);
            push(L, std::string("file exceeds maximum read size"));
            return 2;
        }
        push(L, data);
        return 1;
    }

    int fsWrite(lua_State* L) {
        auto target = resolveTarget(L, "geode.fs.write");
        if (!target) return 2;
        if (!target->writable) return pushReadOnlyError(L);

        auto data = check<std::string>(L, 3, "geode.fs.write");

        auto parent = target->path.parent_path();
        if (!parent.empty()) {
            auto dir = geode::utils::file::createDirectoryAll(parent);
            if (dir.isErr()) {
                lua_pushnil(L);
                push(L, std::string(dir.unwrapErr()));
                return 2;
            }
        }

        auto wrote = geode::utils::file::writeString(target->path, data);
        if (wrote.isErr()) {
            lua_pushnil(L);
            push(L, std::string(wrote.unwrapErr()));
            return 2;
        }
        push(L, true);
        return 1;
    }

    int fsExists(lua_State* L) {
        auto target = resolveTarget(L, "geode.fs.exists");
        if (!target) return 2;

        std::error_code ec;
        bool exists = std::filesystem::exists(target->path, ec);
        push(L, exists && !ec);
        return 1;
    }

    int fsList(lua_State* L) {
        auto target = resolveTarget(L, "geode.fs.list");
        if (!target) return 2;

        auto entries = geode::utils::file::readDirectory(target->path, false);
        if (entries.isErr()) {
            lua_pushnil(L);
            push(L, std::string(entries.unwrapErr()));
            return 2;
        }

        lua_newtable(L);
        int index = 1;
        for (auto const& entry : entries.unwrap()) {
            push(L, geode::utils::string::pathToString(entry.filename()));
            lua_rawseti(L, -2, index++);
        }
        return 1;
    }

    int fsMkdir(lua_State* L) {
        auto target = resolveTarget(L, "geode.fs.mkdir");
        if (!target) return 2;
        if (!target->writable) return pushReadOnlyError(L);

        auto made = geode::utils::file::createDirectoryAll(target->path);
        if (made.isErr()) {
            lua_pushnil(L);
            push(L, std::string(made.unwrapErr()));
            return 2;
        }
        push(L, true);
        return 1;
    }

    int fsRemove(lua_State* L) {
        auto target = resolveTarget(L, "geode.fs.remove");
        if (!target) return 2;
        if (!target->writable) return pushReadOnlyError(L);

        std::error_code ec;
        std::filesystem::remove(target->path, ec);
        if (ec) {
            lua_pushnil(L);
            push(L, ec.message());
            return 2;
        }
        push(L, true);
        return 1;
    }

    geode::Result<void> registerGeodeFs(lua_State* L) {
        luax::getOrCreateTable(L, "geode");
        lua_newtable(L);
        setTableCFunction(L, -1, "read", &fsRead);
        setTableCFunction(L, -1, "write", &fsWrite);
        setTableCFunction(L, -1, "exists", &fsExists);
        setTableCFunction(L, -1, "list", &fsList);
        setTableCFunction(L, -1, "mkdir", &fsMkdir);
        setTableCFunction(L, -1, "remove", &fsRemove);
        lua_setfield(L, -2, "fs");
        lua_pop(L, 1);
        return geode::Ok();
    }
}

LUAX_BINDING(geode_fs_lib, registerGeodeFs)
