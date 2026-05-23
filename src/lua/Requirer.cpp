#include "Requirer.hpp"

#include "Runtime.hpp"

#include <Geode/Geode.hpp>
#include <Luau/CodeGen.h>
#include <Luau/Compiler.h>
#include <lualib.h>

#include <cstring>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

namespace luax {
    namespace {
        luarequire_WriteResult writeString(std::string const& contents, char* buffer, size_t bufferSize, size_t* sizeOut) {
            size_t needed = contents.size() + 1;
            if (bufferSize < needed) {
                *sizeOut = needed;
                return WRITE_BUFFER_TOO_SMALL;
            }
            std::memcpy(buffer, contents.c_str(), needed);
            *sizeOut = needed;
            return WRITE_SUCCESS;
        }

        std::optional<std::string> readFileToString(std::filesystem::path const& path) {
            std::ifstream file(path, std::ios::binary);
            if (!file) return std::nullopt;
            std::ostringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        }

        std::string normalizedPathString(std::filesystem::path const& path) {
            auto out = path.string();
            for (auto& c : out) {
                if (c == '\\') c = '/';
            }
            return out;
        }

        bool escapesRoot(std::filesystem::path const& rel) {
            auto s = rel.generic_string();
            return rel.empty() || s == ".." || s.rfind("../", 0) == 0;
        }

        bool pathInsideRoot(std::filesystem::path const& path, std::filesystem::path const& root) {
            std::error_code ec;
            auto rel = std::filesystem::relative(path, root, ec);
            return !ec && !escapesRoot(rel);
        }

        Requirer* self(void* ctx) { return static_cast<Requirer*>(ctx); }

        bool is_require_allowed(lua_State*, void*, char const* requirer_chunkname) {
            if (!requirer_chunkname) return false;
            return requirer_chunkname[0] == '@';
        }

        luarequire_NavigateResult reset(lua_State*, void* ctx, char const* requirer_chunkname) {
            return self(ctx)->resetTo(requirer_chunkname);
        }

        luarequire_NavigateResult jump_to_alias(lua_State*, void*, char const*) {
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

        luarequire_WriteResult get_chunkname(lua_State*, void* ctx, char* buffer, size_t buffer_size, size_t* size_out) {
            return writeString(self(ctx)->chunkname(), buffer, buffer_size, size_out);
        }

        luarequire_WriteResult get_loadname(lua_State*, void* ctx, char* buffer, size_t buffer_size, size_t* size_out) {
            return writeString(normalizedPathString(self(ctx)->modulePath()), buffer, buffer_size, size_out);
        }

        luarequire_WriteResult get_cache_key(lua_State*, void* ctx, char* buffer, size_t buffer_size, size_t* size_out) {
            return writeString(normalizedPathString(self(ctx)->modulePath()), buffer, buffer_size, size_out);
        }

        luarequire_ConfigStatus get_config_status(lua_State*, void*) {
            return CONFIG_ABSENT;
        }

        luarequire_WriteResult get_config(lua_State*, void*, char*, size_t, size_t*) {
            return WRITE_FAILURE;
        }

        int load(lua_State* L, void* ctx, char const* /*path*/, char const* chunkname, char const* loadname) {
            Requirer* req = self(ctx);

            std::filesystem::path filePath(loadname);
            auto contents = readFileToString(filePath);
            if (!contents) {
                luaL_error(L, "could not read module '%s'", loadname);
            }

            std::string const& bytecode = req->runtime().getOrCompileBytecode(loadname, *contents);

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

            if (req->runtime().codegenEnabled()) {
                Luau::CodeGen::compile(ML, -1, Luau::CodeGen::CodeGen_ColdFunctions);
            }

            int resumeStatus = lua_resume(ML, L, 0);
            if (resumeStatus == 0) {
                if (lua_gettop(ML) != 1) {
                    lua_pop(L, 1);
                    luaL_error(L, "module '%s' must return a single value", chunkname);
                }
            } else if (resumeStatus == LUA_YIELD) {
                lua_pop(L, 1);
                luaL_error(L, "module '%s' yielded", chunkname);
            } else {
                char const* err = lua_tostring(ML, -1);
                std::string msg = err ? err : "(unknown runtime error)";
                lua_pop(L, 1);
                luaL_error(L, "error running module '%s': %s", chunkname, msg.c_str());
            }

            lua_xmove(ML, L, 1);
            lua_remove(L, -2);
            return 1;
        }
    }

    Requirer::Requirer(Runtime& runtime) : m_runtime(runtime) {
        m_root = std::filesystem::weakly_canonical(geode::Mod::get()->getResourcesDir());
    }

    void Requirer::initConfig(luarequire_Configuration* config) {
        if (!config) return;
        config->is_require_allowed = is_require_allowed;
        config->reset              = reset;
        config->jump_to_alias      = jump_to_alias;
        config->to_parent          = to_parent;
        config->to_child           = to_child;
        config->is_module_present  = is_module_present;
        config->get_chunkname      = get_chunkname;
        config->get_loadname       = get_loadname;
        config->get_cache_key      = get_cache_key;
        config->get_config_status  = get_config_status;
        config->get_config         = get_config;
        config->load               = load;
    }

    luarequire_NavigateResult Requirer::resetTo(char const* requirer_chunkname) {
        if (!requirer_chunkname || requirer_chunkname[0] != '@') {
            return NAVIGATE_NOT_FOUND;
        }
        std::string_view rest = requirer_chunkname + 1;
        std::filesystem::path candidate = m_root / rest;
        std::error_code ec;
        auto canonical = std::filesystem::weakly_canonical(candidate, ec);
        if (ec) return NAVIGATE_NOT_FOUND;

        if (!pathInsideRoot(canonical, m_root)) {
            return NAVIGATE_NOT_FOUND;
        }
        m_current = canonical;
        return NAVIGATE_SUCCESS;
    }

    luarequire_NavigateResult Requirer::toParent() {
        if (m_current == m_root) return NAVIGATE_NOT_FOUND;
        auto parent = m_current.parent_path();
        if (!pathInsideRoot(parent, m_root)) return NAVIGATE_NOT_FOUND;
        m_current = parent;
        return NAVIGATE_SUCCESS;
    }

    luarequire_NavigateResult Requirer::toChild(char const* name) {
        if (!name || !*name) return NAVIGATE_NOT_FOUND;
        std::string_view sv = name;
        if (sv.find('/') != std::string_view::npos || sv.find('\\') != std::string_view::npos || sv == "..") {
            return NAVIGATE_NOT_FOUND;
        }
        m_current /= name;
        return NAVIGATE_SUCCESS;
    }

    bool Requirer::isModulePresent() const {
        std::error_code ec;
        auto path = modulePath();
        return std::filesystem::is_regular_file(path, ec);
    }

    std::filesystem::path Requirer::modulePath() const {
        auto path = m_current;
        path += ".luau";
        return path;
    }

    std::string Requirer::chunkname() const {
        std::error_code ec;
        auto rel = std::filesystem::relative(m_current, m_root, ec);
        std::string name = normalizedPathString(ec ? m_current : rel);
        return "@" + name + ".luau";
    }
}
