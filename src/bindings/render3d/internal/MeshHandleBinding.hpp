#pragma once

#include "bindings/render3d/internal/Handles.hpp"

#include <cstdint>
#include <lua.h>
#include <memory>

namespace luax::render3d {
    class MeshAsset;
}

namespace luax::gd3d {
    void registerMeshHandleMetatable(lua_State* L);
    void pushMeshHandle(lua_State* L, std::uint64_t id);
    std::shared_ptr<render3d::MeshAsset> requireMesh(lua_State* L, MeshHandle* handle, char const* method);
} // namespace luax::gd3d
