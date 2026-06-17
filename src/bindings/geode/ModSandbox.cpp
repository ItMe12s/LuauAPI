#include "bindings/geode/ModSandbox.hpp"

#include "bindings/geode/CurrentMod.hpp"
#include "core/Config.hpp"
#include "framework/stack/Stack.hpp"
#include "require/PathSandbox.hpp"

#include <Geode/loader/Mod.hpp>
#include <Geode/utils/file.hpp>
#include <cstdint>
#include <filesystem>
#include <lua.h>
#include <lualib.h>
#include <string>
#include <vector>

namespace {
    geode::Result<void> validateSandboxRegularFile(std::filesystem::path const& path) {
        std::error_code ec;
        if (!std::filesystem::is_regular_file(path, ec)) {
            return geode::Err("path is not a regular file");
        }
        return geode::Ok();
    }

    geode::Result<void> validateSandboxReadSize(std::uintmax_t size) {
        if (size > luax::kMaxFsReadBytes) {
            return geode::Err("file exceeds maximum read size");
        }
        return geode::Ok();
    }

    geode::Result<void> validateSandboxReadableFile(std::filesystem::path const& path) {
        auto valid = validateSandboxRegularFile(path);
        if (valid.isErr()) {
            return geode::Err(valid.unwrapErr());
        }

        std::error_code ec;
        auto size = std::filesystem::file_size(path, ec);
        if (ec) {
            return geode::Err("file cannot be read");
        }
        return validateSandboxReadSize(size);
    }

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

    geode::Result<std::string> readSandboxTextFile(std::filesystem::path const& path) {
        auto valid = validateSandboxReadableFile(path);
        if (valid.isErr()) {
            return geode::Err(valid.unwrapErr());
        }

        auto contents = geode::utils::file::readString(path);
        if (contents.isErr()) {
            return geode::Err(contents.unwrapErr());
        }
        auto data = std::move(contents.unwrap());
        auto sizeCheck = validateSandboxReadSize(data.size());
        if (sizeCheck.isErr()) {
            return geode::Err(sizeCheck.unwrapErr());
        }
        return geode::Ok(std::move(data));
    }

    geode::Result<std::vector<std::uint8_t>> readSandboxBinaryFile(std::filesystem::path const& path) {
        auto valid = validateSandboxReadableFile(path);
        if (valid.isErr()) {
            return geode::Err(valid.unwrapErr());
        }

        auto contents = geode::utils::file::readBinary(path);
        if (contents.isErr()) {
            return geode::Err(contents.unwrapErr());
        }
        auto data = std::move(contents.unwrap());
        auto sizeCheck = validateSandboxReadSize(data.size());
        if (sizeCheck.isErr()) {
            return geode::Err(sizeCheck.unwrapErr());
        }
        return geode::Ok(std::move(data));
    }
} // namespace luax
