#pragma once

#include "bindings/render3d/internal/Handles.hpp"

#include <lua.h>
#include <memory>

namespace luax::render3d {
    class MeshAsset;
}

namespace luax::gd3d {
    void registerMeshHandleMetatable(lua_State* L);
} // namespace luax::gd3d
