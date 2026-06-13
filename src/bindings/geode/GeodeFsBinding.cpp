#include "bindings/geode/ModSandbox.hpp"
#include "core/Config.hpp"
#include "framework/Binding.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"

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

        auto contents = readSandboxTextFile(target->path);
        if (contents.isErr()) {
            return pushNilErr(L, contents.unwrapErr());
        }
        push(L, contents.unwrap());
        return 1;
    }

    int fsWrite(lua_State* L) {
        auto target = resolveSandboxTarget(L, 1, 2, "geode.fs.write", true);
        if (!target) return 2;

        auto data = check<std::string>(L, 3, "geode.fs.write");
        if (data.size() > kMaxFsWriteBytes) {
            return pushNilErr(L, "data exceeds maximum write size");
        }

        auto parent = target->path.parent_path();
        if (!parent.empty()) {
            auto dir = geode::utils::file::createDirectoryAll(parent);
            if (dir.isErr()) {
                return pushNilErr(L, dir.unwrapErr());
            }
        }

        auto wrote = geode::utils::file::writeString(target->path, data);
        if (wrote.isErr()) {
            return pushNilErr(L, wrote.unwrapErr());
        }
        push(L, true);
        return 1;
    }

    int fsExists(lua_State* L) {
        auto target = resolveSandboxTarget(L, 1, 2, "geode.fs.exists");
        if (!target) return 2;

        std::error_code ec;
        bool exists = std::filesystem::exists(target->path, ec);
        if (ec) {
            return pushNilErr(L, ec.message());
        }
        push(L, exists);
        return 1;
    }

    int fsList(lua_State* L) {
        auto target = resolveSandboxTarget(L, 1, 2, "geode.fs.list");
        if (!target) return 2;

        auto entries = geode::utils::file::readDirectory(target->path, false);
        if (entries.isErr()) {
            return pushNilErr(L, entries.unwrapErr());
        }

        lua_newtable(L);
        int index = 1;
        std::size_t totalNameBytes = 0;
        for (auto const& entry : entries.unwrap()) {
            if (static_cast<std::size_t>(index) > kMaxFsListEntries) {
                lua_pop(L, 1);
                return pushNilErr(L, "directory listing exceeds maximum entries");
            }
            auto name = geode::utils::string::pathToString(entry.filename());
            if (totalNameBytes + name.size() > kMaxFsListNameBytes) {
                lua_pop(L, 1);
                return pushNilErr(L, "directory listing exceeds maximum name bytes");
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
            return pushNilErr(L, made.unwrapErr());
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
            return pushNilErr(L, ec.message());
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
