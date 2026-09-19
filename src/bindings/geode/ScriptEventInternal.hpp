#pragma once

#include <Geode/loader/Event.hpp>
#include <string>
#include <string_view>

namespace luax {
    struct LuaListenerEventTag {};

    struct LuaListenerForEventTag {};

    using LuaListenerEvent =
        geode::Event<LuaListenerEventTag, bool(std::string_view, std::string_view)>;
    using LuaListenerForEvent =
        geode::Event<LuaListenerForEventTag, bool(std::string_view, std::string_view), std::string>;

    inline void dispatchToLuaListeners(std::string_view topic, std::string_view payload) {
        if (LuaListenerEvent().send(topic, payload)) return;
        LuaListenerForEvent(std::string(topic)).send(topic, payload);
    }
} // namespace luax
