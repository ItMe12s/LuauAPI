#include "framework/BindingHost.hpp"

#include "core/Runtime.hpp"

namespace luax {
    BindingHost* BindingHost::getIfInitialized() {
        return Runtime::getIfInitialized();
    }

    BindingHost* BindingHost::fromState(lua_State* L) {
        if (!L) return nullptr;
        return static_cast<BindingHost*>(lua_callbacks(L)->userdata);
    }

    BindingHost::ResourcesRootScope::ResourcesRootScope(
        BindingHost& host, std::filesystem::path const& root
    ) : m_host(host) {
        if (host.resourcesRoot() == root) {
            return;
        }
        std::filesystem::path next = root;
        host.swapResourcesRoot(next);
        m_saved.emplace(std::move(next));
    }

    BindingHost::ResourcesRootScope::~ResourcesRootScope() {
        if (!m_saved) {
            return;
        }
        std::filesystem::path restore = std::move(*m_saved);
        m_host.swapResourcesRoot(restore);
    }
} // namespace luax
