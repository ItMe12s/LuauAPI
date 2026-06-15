#include "require/Requirer.hpp"

#include "core/Runtime.hpp"
#include "require/BytecodeCacheKey.hpp"
#include "require/PathSandbox.hpp"
#include "require/VirtualChunk.hpp"
#include "require/RequirePath.hpp"

#include <Luau/CodeGen.h>
#include <Luau/Compiler.h>
#include <cstring>
#include <functional>
#include <lualib.h>
#include <string>
#include <string_view>

namespace luax {
    namespace {
        luarequire_WriteResult writeString(
            std::string const& contents, char* buffer, size_t bufferSize, size_t* sizeOut
        ) {
            size_t needed = contents.size() + 1;
            if (bufferSize < needed) {
                *sizeOut = needed;
                return WRITE_BUFFER_TOO_SMALL;
            }
            std::memcpy(buffer, contents.c_str(), needed);
            *sizeOut = needed;
            return WRITE_SUCCESS;
        }

        Requirer* self(void* ctx) {
            return static_cast<Requirer*>(ctx);
        }

        bool is_require_allowed(lua_State*, void*, char const* requirer_chunkname) {
            return requirer_chunkname && canRequireFromChunk(requirer_chunkname);
        }

        luarequire_NavigateResult reset(lua_State*, void* ctx, char const* requirer_chunkname) {
            return self(ctx)->resetTo(requirer_chunkname);
        }

        luarequire_NavigateResult jump_to_alias(lua_State*, void*, char const* alias) {
            geode::log::debug("require alias '{}' not supported", alias ? alias : "");
            return NAVIGATE_NOT_FOUND;
        }

        luarequire_NavigateResult to_parent(lua_State*, void* ctx) {
            return self(ctx)->toParent();
        }

        luarequire_NavigateResult to_child(lua_State*, void* ctx, char const* name) {
            return self(ctx)->toChild(name);
        }

        bool is_module_present(lua_State*, void* ctx) {
            return self(ctx)->isModulePresent();
        }

        luarequire_WriteResult get_chunkname(
            lua_State*, void* ctx, char* buffer, size_t buffer_size, size_t* size_out
        ) {
            auto const& name = self(ctx)->chunkname();
            if (name.empty()) {
                return WRITE_FAILURE;
            }
            return writeString(name, buffer, buffer_size, size_out);
        }

        luarequire_WriteResult get_loadname(
            lua_State*, void* ctx, char* buffer, size_t buffer_size, size_t* size_out
        ) {
            auto path = self(ctx)->resolvedModulePath();
            if (path.isErr()) return WRITE_FAILURE;
            return writeString(filesystemPathString(path.unwrap()), buffer, buffer_size, size_out);
        }

        luarequire_WriteResult get_cache_key(
            lua_State*, void* ctx, char* buffer, size_t buffer_size, size_t* size_out
        ) {
            return self(ctx)->writeCacheKey(buffer, buffer_size, size_out);
        }

        luarequire_ConfigStatus get_config_status(lua_State*, void*) {
            return CONFIG_ABSENT;
        }

        luarequire_WriteResult get_config(lua_State*, void*, char*, size_t, size_t*) {
            return WRITE_FAILURE;
        }

        int load(
            lua_State* L, void* ctx, char const* /*path*/, char const* chunkname, char const* loadname
        ) {
            return self(ctx)->loadModule(L, chunkname, loadname);
        }
    } // namespace

    Requirer::Requirer(Runtime& runtime) : m_runtime(runtime) {}

    void Requirer::clearPendingLoad() {
        m_pendingLoadPath.clear();
        m_pendingLoadContents.clear();
    }

    void Requirer::cachePendingLoad(std::filesystem::path const& path, std::string contents) {
        m_pendingLoadPath = path;
        m_pendingLoadContents = std::move(contents);
    }

    geode::Result<std::string const&> Requirer::pendingLoadContents(
        std::filesystem::path const& path
    ) const {
        if (m_pendingLoadPath.empty() || path != m_pendingLoadPath) {
            return geode::Err("pending module contents unavailable");
        }
        return geode::Ok(std::cref(m_pendingLoadContents));
    }

    void Requirer::setResourcesRoot(std::filesystem::path const& root) {
        if (root.empty()) {
            m_root.clear();
            m_rootError.reset();
            m_current.clear();
            clearPendingLoad();
            return;
        }
        auto rootResult = canonicalRoot(root);
        if (rootResult.isErr()) {
            geode::log::error(
                "require: resources root canonicalization failed: {}", rootResult.unwrapErr()
            );
            m_root.clear();
            m_rootError = rootResult.unwrapErr();
            m_current.clear();
            clearPendingLoad();
            return;
        }
        m_root = rootResult.unwrap();
        m_rootError.reset();
        m_current = m_root;
        clearPendingLoad();
    }

    void Requirer::initConfig(luarequire_Configuration* config) {
        if (!config) return;
        config->is_require_allowed = is_require_allowed;
        config->reset = reset;
        config->jump_to_alias = jump_to_alias;
        config->to_parent = to_parent;
        config->to_child = to_child;
        config->is_module_present = is_module_present;
        config->get_chunkname = get_chunkname;
        config->get_loadname = get_loadname;
        config->get_cache_key = get_cache_key;
        config->get_config_status = get_config_status;
        config->get_config = get_config;
        config->load = load;
    }

    luarequire_NavigateResult Requirer::resetTo(char const* requirer_chunkname) {
        if (m_root.empty()) {
            return NAVIGATE_NOT_FOUND;
        }
        if (!requirer_chunkname || requirer_chunkname[0] != '@') {
            return NAVIGATE_NOT_FOUND;
        }
        auto scriptPath = virtualChunkScriptPath(requirer_chunkname);
        auto validated = validateResourcePath(std::filesystem::path(scriptPath), false);
        if (validated.isErr()) {
            return NAVIGATE_NOT_FOUND;
        }

        auto resolved = resolveInsideRoot(m_root, normalizedPathString(validated.unwrap()));
        if (resolved.isErr()) {
            return NAVIGATE_NOT_FOUND;
        }
        m_current = resolved.unwrap();
        clearPendingLoad();
        return NAVIGATE_SUCCESS;
    }

    luarequire_NavigateResult Requirer::toParent() {
        if (m_root.empty()) {
            return NAVIGATE_NOT_FOUND;
        }
        if (m_current == m_root) return NAVIGATE_NOT_FOUND;
        auto parent = m_current.parent_path();
        if (!pathInsideRoot(parent, m_root)) return NAVIGATE_NOT_FOUND;
        m_current = parent;
        clearPendingLoad();
        return NAVIGATE_SUCCESS;
    }

    luarequire_NavigateResult Requirer::toChild(char const* name) {
        if (m_root.empty()) {
            return NAVIGATE_NOT_FOUND;
        }
        if (!name || !isRequireChildNameAllowed(name)) {
            return NAVIGATE_NOT_FOUND;
        }
        if (m_current != m_root) return NAVIGATE_NOT_FOUND;
        auto next = (m_root / name).lexically_normal();
        m_current = next;
        clearPendingLoad();
        return NAVIGATE_SUCCESS;
    }

    bool Requirer::isModulePresent() const {
        std::error_code ec;
        auto resolved = resolvedModulePath();
        if (resolved.isErr()) return false;
        auto path = resolved.unwrap();
        return std::filesystem::is_regular_file(path, ec);
    }

    std::filesystem::path Requirer::modulePath() const {
        return requireModulePath(m_current);
    }

    geode::Result<std::filesystem::path> Requirer::resolvedModulePath() const {
        if (m_root.empty()) {
            if (m_rootError) {
                return geode::Err(*m_rootError);
            }
            return geode::Err("resources root is not configured");
        }
        auto resolved = resolveScriptFileInsideRoot(m_root, modulePath());
        if (resolved.isErr()) {
            return geode::Err(resolved.unwrapErr());
        }
        return geode::Ok(resolved.unwrap());
    }

    std::string Requirer::chunkname() const {
        if (m_root.empty()) {
            return {};
        }
        std::error_code ec;
        auto rel = std::filesystem::relative(m_current, m_root, ec);
        std::string name = normalizedPathString(ec ? m_current : rel);
        auto path = std::filesystem::path(name);
        if (!hasLuauExtension(path)) {
            name += ".luau";
        }
        if (auto modId = modIdForResourcesRoot(m_root)) {
            return formatVirtualChunk(*modId, name);
        }
        return "@" + name;
    }

    luarequire_WriteResult Requirer::writeCacheKey(char* buffer, size_t bufferSize, size_t* sizeOut) {
        auto path = resolvedModulePath();
        if (path.isErr()) return WRITE_FAILURE;
        auto filePath = path.unwrap();

        auto contentsResult = readScriptFile(filePath);
        if (contentsResult.isErr()) return WRITE_FAILURE;

        auto const& contents = contentsResult.unwrap();
        cachePendingLoad(filePath, contents);
        return writeString(fileCacheKey(filePath, contents), buffer, bufferSize, sizeOut);
    }

    int Requirer::loadModule(lua_State* L, char const* chunkname, char const* loadname) {
        auto pathResult = resolvedModulePath();
        if (pathResult.isErr()) {
            luaL_error(
                L, "module '%s' cannot be resolved: %s", loadname, pathResult.unwrapErr().c_str()
            );
        }
        auto filePath = pathResult.unwrap();

        std::error_code ec;
        auto fileSize = std::filesystem::file_size(filePath, ec);
        if (ec || fileSize > kMaxScriptBytes) {
            luaL_error(
                L,
                "module '%s' exceeds maximum size or cannot be read",
                filesystemPathString(filePath).c_str()
            );
        }

        geode::Result<std::string const&> contentsResult = pendingLoadContents(filePath);
        std::string fallbackContents;
        if (contentsResult.isErr()) {
            auto readResult = readScriptFile(filePath);
            if (readResult.isErr()) {
                luaL_error(
                    L,
                    "could not read module '%s': %s",
                    filesystemPathString(filePath).c_str(),
                    readResult.unwrapErr().c_str()
                );
            }
            fallbackContents = readResult.unwrap();
            contentsResult = geode::Ok(std::cref(fallbackContents));
        }
        auto const& contents = contentsResult.unwrap();
        auto bytecodeResult =
            m_runtime.getOrCompileBytecode(fileCacheKey(filePath, contents), contents);
        if (bytecodeResult.isErr()) {
            luaL_error(
                L, "module '%s' compile failed: %s", chunkname, bytecodeResult.unwrapErr().c_str()
            );
        }
        auto const& bytecode = bytecodeResult.unwrap().get();
        clearPendingLoad();

        lua_State* GL = lua_mainthread(L);
        lua_State* ML = lua_newthread(GL);
        lua_xmove(GL, L, 1);

        luaL_sandboxthread(ML);

        int loadStatus = luau_load(ML, chunkname, bytecode.data(), bytecode.size(), 0);
        if (loadStatus != 0) {
            char const* err = lua_tostring(ML, -1);
            std::string msg = err ? err : "(unknown load error)";
            lua_pop(L, 1);
            luaL_error(L, "module '%s' failed to load: %s", chunkname, msg.c_str());
        }

        if (m_runtime.codegenEnabled()) {
            Luau::CodeGen::compile(ML, -1, Luau::CodeGen::CodeGen_ColdFunctions);
        }

        int resumeStatus = 0;
        {
            Runtime::ScriptBudgetGuard budget(m_runtime, kDefaultScriptDeadlineMs);
            resumeStatus = lua_resume(ML, L, 0);
        }
        if (resumeStatus == 0) {
            if (lua_gettop(ML) != 1) {
                lua_pop(L, 1);
                luaL_error(L, "module '%s' must return a single value", chunkname);
            }
        }
        else if (resumeStatus == LUA_YIELD) {
            lua_pop(L, 1);
            luaL_error(L, "module '%s' yielded", chunkname);
        }
        else {
            char const* err = lua_tostring(ML, -1);
            std::string msg = err ? err : "(unknown runtime error)";
            lua_pop(L, 1);
            luaL_error(L, "error running module '%s': %s", chunkname, msg.c_str());
        }

        lua_xmove(ML, L, 1);
        lua_remove(L, -2);
        return 1;
    }
} // namespace luax
