#include "lua/bindings/framework/BindingHost.hpp"

#include "lua/runtime/Runtime.hpp"

namespace luax {
    BindingHost* BindingHost::getIfInitialized() {
        return Runtime::getIfInitialized();
    }

    BindingHost* BindingHost::fromState(lua_State* L) {
        if (!L) return nullptr;
        return static_cast<BindingHost*>(lua_callbacks(L)->userdata);
    }

    BindingHost::ResourcesRootScope::ResourcesRootScope(BindingHost& host, std::filesystem::path root)
        : m_host(host), m_previous(host.resourcesRoot()) {
        m_host.setResourcesRoot(root);
    }

    BindingHost::ResourcesRootScope::~ResourcesRootScope() {
        m_host.setResourcesRoot(m_previous);
    }
}
