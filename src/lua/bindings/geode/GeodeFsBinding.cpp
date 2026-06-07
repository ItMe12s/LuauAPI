#include "lua/Config.hpp"
#include "lua/bindings/Binding.hpp"
#include "lua/bindings/framework/Stack.hpp"
#include "lua/bindings/framework/TableUtil.hpp"
#include "lua/bindings/geode/ModSandbox.hpp"

#include <Geode/utils/file.hpp>
#include <Geode/utils/string.hpp>
#include <filesystem>
#include <lua.h>
#include <lualib.h>
#include <string>
#include <vector>

namespace {
    using namespace luax;

    int fsRead(lua_State* L) {
        auto target = resolveSandboxTarget(L, 1, 2, "geode.fs.read");
        if (!target) return 2;

        std::error_code ec;
        if (!std::filesystem::is_regular_file(target->path, ec)) {
            lua_pushnil(L);
            push(L, std::string("path is not a regular file"));
            return 2;
        }

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
        auto target = resolveSandboxTarget(L, 1, 2, "geode.fs.write", true);
        if (!target) return 2;

        auto data = check<std::string>(L, 3, "geode.fs.write");
        if (data.size() > kMaxFsWriteBytes) {
            lua_pushnil(L);
            push(L, std::string("data exceeds maximum write size"));
            return 2;
        }

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
        auto target = resolveSandboxTarget(L, 1, 2, "geode.fs.exists");
        if (!target) return 2;

        std::error_code ec;
        bool exists = std::filesystem::exists(target->path, ec);
        push(L, exists && !ec);
        return 1;
    }

    int fsList(lua_State* L) {
        auto target = resolveSandboxTarget(L, 1, 2, "geode.fs.list");
        if (!target) return 2;

        auto entries = geode::utils::file::readDirectory(target->path, false);
        if (entries.isErr()) {
            lua_pushnil(L);
            push(L, std::string(entries.unwrapErr()));
            return 2;
        }

        lua_newtable(L);
        int index = 1;
        std::size_t totalNameBytes = 0;
        for (auto const& entry : entries.unwrap()) {
            if (static_cast<std::size_t>(index) > kMaxFsListEntries) {
                lua_pop(L, 1);
                lua_pushnil(L);
                push(L, std::string("directory listing exceeds maximum entries"));
                return 2;
            }
            auto name = geode::utils::string::pathToString(entry.filename());
            if (totalNameBytes + name.size() > kMaxFsListNameBytes) {
                lua_pop(L, 1);
                lua_pushnil(L);
                push(L, std::string("directory listing exceeds maximum name bytes"));
                return 2;
            }
            totalNameBytes += name.size();
            push(L, std::move(name));
            lua_rawseti(L, -2, index++);
        }
        return 1;
    }

    int fsMkdir(lua_State* L) {
        auto target = resolveSandboxTarget(L, 1, 2, "geode.fs.mkdir", true);
        if (!target) return 2;

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
        auto target = resolveSandboxTarget(L, 1, 2, "geode.fs.remove", true);
        if (!target) return 2;

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

} // namespace

namespace luax {
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
} // namespace luax

#if !defined(LUAUAPI_HOST_TESTS)
LUAX_BINDING(geode_fs_lib, registerGeodeFs)
#endif
