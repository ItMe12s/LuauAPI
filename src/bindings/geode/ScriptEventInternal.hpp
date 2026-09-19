#pragma once

#include <Geode/loader/Event.hpp>
#include <string>
#include <string_view>

namespace luax {
    struct ScriptLuaListenTag {};

    struct ScriptLuaListenForTag {};

    using ScriptLuaListen =
        geode::Event<ScriptLuaListenTag, bool(std::string_view, std::string_view)>;
    using ScriptLuaListenFor =
        geode::Event<ScriptLuaListenForTag, bool(std::string_view, std::string_view), std::string>;

    inline void postScriptEventToLua(std::string_view topic, std::string_view payload) {
        ScriptLuaListen().send(topic, payload);
        ScriptLuaListenFor(std::string(topic)).send(topic, payload);
    }
} // namespace luax