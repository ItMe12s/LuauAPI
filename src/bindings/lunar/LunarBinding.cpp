#include "bindings/lunar/LunarAnimation.hpp"
#include "bindings/lunar/LunarRig.hpp"
#include "core/Runtime.hpp"
#include "framework/Binding.hpp"

#include <Geode/Geode.hpp>
#include <lua.h>

namespace luax {
    geode::Result<void> registerLunar(lua_State* L) {
        auto rigResult = lunar::registerLunarRig(L);
        if (rigResult.isErr()) return rigResult;
        auto animResult = lunar::registerLunarAnimation(L);
        if (animResult.isErr()) return animResult;

        if (auto* runtime = static_cast<Runtime*>(lua_callbacks(L)->userdata)) {
            runtime->registerShutdownHook([] {
                lunar::shutdownLunarTracks();
            });
        }
        return geode::Ok();
    }
} // namespace luax

#if !defined(LUAUAPI_HOST_TESTS)
LUAX_BINDING(lunar_lib, luax::registerLunar)
#endif
