#pragma once

#include "lua/Config.hpp"

#include <filesystem>
#include <functional>
#include <lua.h>
#include <optional>
#include <string_view>

namespace luax {
    class BindingHost {
    public:
        virtual ~BindingHost() = default;

        static BindingHost* getIfInitialized();
        static BindingHost* fromState(lua_State* L);

        virtual lua_State* state() = 0;
        virtual bool ready() const = 0;
        virtual bool protectedCall(
            int nargs, int nresults, std::string_view context, int deadlineMs = kDefaultScriptDeadlineMs
        ) = 0;
        virtual bool protectedCallWithTraceback(int nargs, int nresults, std::string_view context) = 0;
        virtual void registerShutdownHook(std::function<void()> fn) = 0;
        virtual void setResourcesRoot(std::filesystem::path const& root) = 0;
        virtual void swapResourcesRoot(std::filesystem::path& root) = 0;
        virtual std::filesystem::path const& resourcesRoot() const = 0;

        class ResourcesRootScope final {
        public:
            ResourcesRootScope(BindingHost& host, std::filesystem::path const& root);
            ~ResourcesRootScope();

            ResourcesRootScope(ResourcesRootScope const&) = delete;
            ResourcesRootScope& operator=(ResourcesRootScope const&) = delete;

        private:
            BindingHost& m_host;
            std::optional<std::filesystem::path> m_saved;
        };
    };
} // namespace luax
