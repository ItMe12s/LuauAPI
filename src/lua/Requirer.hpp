#pragma once

#include <Luau/Require.h>
#include <lua.h>

#include <filesystem>

namespace luax {
    class Runtime;

    class Requirer final {
    public:
        explicit Requirer(Runtime& runtime);

        static void initConfig(luarequire_Configuration* config);

        Runtime& runtime() { return m_runtime; }

        std::filesystem::path const& resourcesRoot() const { return m_root; }
        std::filesystem::path const& current() const { return m_current; }

        luarequire_NavigateResult resetTo(char const* requirerChunkname);
        luarequire_NavigateResult toParent();
        luarequire_NavigateResult toChild(char const* name);

        bool isModulePresent() const;
        std::filesystem::path modulePath() const;
        std::string chunkname() const;

    private:
        Runtime& m_runtime;
        std::filesystem::path m_root;
        std::filesystem::path m_current;
    };
}
