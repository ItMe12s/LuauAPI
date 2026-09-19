#pragma once

#include "Export.hpp"

#include <Geode/loader/Event.hpp>
#include <string_view>

namespace imes::luauapi {
    struct LuaScriptEventTag {};

    using LuaScriptEvent = geode::Event<LuaScriptEventTag, bool(std::string_view, std::string_view)>;

    LUAUAPI_DLL void postScriptEvent(std::string_view topic, std::string_view payload = {});
} // namespace imes::luauapi