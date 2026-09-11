#pragma once

#include <Geode/Result.hpp>
#include <Luau/Require.h>
#include <filesystem>
#include <lua.h>
#include <optional>
#include <string>

namespace luax {
    class Runtime;

    class Requirer final {
    public:
        explicit Requirer(Runtime& runtime);

        static void initConfig(luarequire_Configuration* config);

        Runtime& runtime() {
            return m_runtime;
        }

        void setResourcesRoot(std::filesystem::path const& root);

        std::filesystem::path const& resourcesRoot() const {
            return m_root;
        }

        std::filesystem::path const& current() const {
            return m_current;
        }

        luarequire_NavigateResult resetTo(char const* requirerChunkname);
        luarequire_NavigateResult toParent();
        luarequire_NavigateResult toChild(char const* name);

        bool isModulePresent() const;
        std::filesystem::path modulePath() const;
        geode::Result<std::filesystem::path> resolvedModulePath() const;
        std::string chunkname() const;

        luarequire_WriteResult writeCacheKey(char* buffer, size_t bufferSize, size_t* sizeOut);
        int loadModule(lua_State* L, char const* chunkname, char const* loadname);

    private:
        struct ResolvedModule {
            std::filesystem::path filePath;
            std::string fallback;
            std::string const* contents = nullptr;
        };

        void resolveAndRead(lua_State* L, char const* loadname, ResolvedModule& out);
        void compileModule(
            lua_State* L, char const* chunkname, ResolvedModule& resolved, std::string const*& bytecode
        );
        void execLoadedModule(lua_State* L, char const* chunkname, std::string const& bytecode);

        void clearPendingLoad();
        void cachePendingLoad(std::filesystem::path const& path, std::string contents);
        geode::Result<std::string const&> pendingLoadContents(std::filesystem::path const& path) const;

        Runtime& m_runtime;
        std::filesystem::path m_root;
        std::optional<std::string> m_rootError;
        std::filesystem::path m_current;
        std::filesystem::path m_pendingLoadPath;
        std::string m_pendingLoadContents;
        std::string m_pendingLoadKey;
    };
} // namespace luax
