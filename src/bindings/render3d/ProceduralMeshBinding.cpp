#include "bindings/render3d/internal/Marshaling.hpp"
#include "bindings/render3d/internal/MeshHandleBinding.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"
#include "render3d/assets/MeshAsset.hpp"

#include <Geode/Geode.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <lua.h>
#include <lualib.h>
#include <string>
#include <vector>

namespace {
    using namespace luax;
    using namespace luax::gd3d;
    using namespace luax::render3d;

    bool readVec3Array(
        lua_State* L, int tableIdx, char const* field, char const* method,
        std::vector<glm::vec3>& out, std::string& err
    ) {
        lua_getfield(L, tableIdx, field);
        if (!lua_istable(L, -1)) {
            err = std::string(method) + ": " + field + " must be a table";
            lua_pop(L, 1);
            return false;
        }

        int const len = lua_objlen(L, -1);
        if (len <= 0) {
            err = std::string(method) + ": " + field + " is empty";
            lua_pop(L, 1);
            return false;
        }

        out.clear();
        out.reserve(static_cast<std::size_t>(len));
        for (int i = 1; i <= len; ++i) {
            lua_rawgeti(L, -1, i);
            if (!lua_istable(L, -1)) {
                err = std::string(method) + ": " + field + " entries must be Vec3 tables";
                lua_pop(L, 2);
                return false;
            }

            out.push_back(checkVec3(L, -1, method));
            lua_pop(L, 1);
        }

        lua_pop(L, 1);
        return true;
    }

    bool readOptionalVec3Array(
        lua_State* L, int tableIdx, char const* field, char const* method,
        std::vector<glm::vec3>& out, std::string& err
    ) {
        lua_getfield(L, tableIdx, field);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            out.clear();
            return true;
        }

        if (!lua_istable(L, -1)) {
            err = std::string(method) + ": " + field + " must be a table when provided";
            lua_pop(L, 1);
            return false;
        }

        int const len = lua_objlen(L, -1);
        out.clear();
        out.reserve(static_cast<std::size_t>(len));
        for (int i = 1; i <= len; ++i) {
            lua_rawgeti(L, -1, i);
            if (!lua_istable(L, -1)) {
                err = std::string(method) + ": " + field + " entries must be Vec3 tables";
                lua_pop(L, 2);
                return false;
            }

            out.push_back(checkVec3(L, -1, method));
            lua_pop(L, 1);
        }

        lua_pop(L, 1);
        return true;
    }

    bool readVec2Array(
        lua_State* L, int tableIdx, char const* field, char const* method,
        std::vector<glm::vec2>& out, std::string& err
    ) {
        lua_getfield(L, tableIdx, field);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            out.clear();
            return true;
        }

        if (!lua_istable(L, -1)) {
            err = std::string(method) + ": " + field + " must be a table when provided";
            lua_pop(L, 1);
            return false;
        }

        int const len = lua_objlen(L, -1);
        out.clear();
        out.reserve(static_cast<std::size_t>(len));
        for (int i = 1; i <= len; ++i) {
            lua_rawgeti(L, -1, i);
            if (!lua_istable(L, -1)) {
                err = std::string(method) + ": " + field + " entries must be { x, y } tables";
                lua_pop(L, 2);
                return false;
            }

            out.emplace_back(fieldNumber(L, -1, "x", method), fieldNumber(L, -1, "y", method));
            lua_pop(L, 1);
        }

        lua_pop(L, 1);
        return true;
    }

    bool readIndexArray(
        lua_State* L, int tableIdx, char const* field, char const* method,
        std::vector<std::uint32_t>& out, std::string& err
    ) {
        lua_getfield(L, tableIdx, field);
        if (!lua_istable(L, -1)) {
            err = std::string(method) + ": " + field + " must be a table";
            lua_pop(L, 1);
            return false;
        }

        int const len = lua_objlen(L, -1);
        if (len <= 0) {
            err = std::string(method) + ": " + field + " is empty";
            lua_pop(L, 1);
            return false;
        }

        out.clear();
        out.reserve(static_cast<std::size_t>(len));
        for (int i = 1; i <= len; ++i) {
            lua_rawgeti(L, -1, i);
            if (!lua_isnumber(L, -1)) {
                err = std::string(method) + ": " + field + " entries must be numbers";
                lua_pop(L, 2);
                return false;
            }

            double const raw = lua_tonumber(L, -1);
            if (raw < 1.0 || raw > static_cast<double>(kMaxProceduralMeshVertices) ||
                static_cast<double>(static_cast<std::uint32_t>(raw)) != raw) {
                err =
                    std::string(method) + ": " + field + " entries must be 1-based integer indices";
                lua_pop(L, 2);
                return false;
            }

            out.push_back(static_cast<std::uint32_t>(static_cast<int>(raw) - 1));
            lua_pop(L, 1);
        }

        lua_pop(L, 1);
        return true;
    }

    int meshNew(lua_State* L) {
        char const* method = "gd3d.mesh.new";
        luaL_checktype(L, 1, LUA_TTABLE);

        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> uvs;
        std::vector<std::uint32_t> indices;
        std::string err;

        if (!readVec3Array(L, 1, "positions", method, positions, err)) {
            return pushNilErr(L, err);
        }

        if (positions.size() > kMaxProceduralMeshVertices) {
            return pushNilErr(L, "positions exceed maximum vertex count");
        }

        if (!readIndexArray(L, 1, "indices", method, indices, err)) {
            return pushNilErr(L, err);
        }

        if (!readOptionalVec3Array(L, 1, "normals", method, normals, err)) {
            return pushNilErr(L, err);
        }

        if (!readVec2Array(L, 1, "uvs", method, uvs, err)) {
            return pushNilErr(L, err);
        }

        auto result = MeshAsset::fromBuffers(
            std::move(positions), std::move(normals), std::move(uvs), std::move(indices)
        );
        if (result.isErr()) {
            return pushNilErr(L, result.unwrapErr());
        }

        auto const id = MeshRegistry::instance().registerMesh(result.unwrap());
        pushMeshHandle(L, id);
        return 1;
    }
} // namespace

namespace luax {
    geode::Result<void> registerProceduralMesh(lua_State* L) {
        registerMeshHandleMetatable(L);

        getOrCreateTable(L, "gd3d.mesh");
        setTableCFunction(L, -1, "new", &meshNew);
        lua_pop(L, 1);

        return geode::Ok();
    }
} // namespace luax
