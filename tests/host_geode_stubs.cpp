#include <Geode/loader/Loader.hpp>
#include <Geode/loader/Log.hpp>
#include <Geode/loader/ModEvent.hpp>
#include "lua/bindings/internal/Fields.hpp"
#include <cocos2d.h>

namespace geode {
    Loader* Loader::get() {
        alignas(Loader) static unsigned char storage[sizeof(Loader)]{};
        return reinterpret_cast<Loader*>(storage);
    }

    void Loader::queueInMainThread(ScheduledFunction&& func) {
        if (func) {
            func();
        }
    }

    Mod* getMod() {
        return nullptr;
    }
}

namespace geode::log {
    void vlogImpl(Severity, Mod*, fmt::string_view, fmt::format_args) {}
}

namespace cocos2d {
    void CCObject::retain() {
        ++m_uReference;
    }

    void CCObject::release() {
        luax::Fields::evictIfFinalRelease(this);
        if (m_uReference > 0) {
            --m_uReference;
        }
    }
}
