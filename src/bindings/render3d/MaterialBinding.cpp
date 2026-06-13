#include "bindings/render3d/internal/Handles.hpp"
#include "bindings/render3d/internal/Marshaling.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"
#include "framework/stack/TaggedMetatable.hpp"
#include "framework/stack/UserdataTags.hpp"
#include "render3d/assets/TextureAsset.hpp"
#include "render3d/types/Material.hpp"

#include <Geode/Geode.hpp>
#include <lua.h>
#include <lualib.h>
#include <memory>

namespace {
    using namespace luax;
    using namespace luax::gd3d;

    int materialBaseColor(lua_State* L) {
        auto const& material = requireMaterial(L, 1, "Material:baseColor");
        pushColor(L, material->baseColorFactor);
        return 1;
    }

    int materialHasTexture(lua_State* L) {
        auto const& material = requireMaterial(L, 1, "Material:hasTexture");
        bool const hasStandaloneTexture = material->textureId != 0 && material->texture != nullptr;
        bool const hasGltfTexture = material->imageIndex >= 0 && material->sourceMesh != nullptr;
        push(L, hasStandaloneTexture || hasGltfTexture);
        return 1;
    }

    int materialNew(lua_State* L) {
        luaL_checktype(L, 1, LUA_TTABLE);
        lua_getfield(L, 1, "color");
        if (lua_isnil(L, -1)) {
            luaL_error(L, "gd3d.Material.new: expected color field");
        }
        auto const color = parseColor(L, lua_gettop(L), "gd3d.Material.new");
        lua_pop(L, 1);

        auto material = std::make_shared<render3d::Material>();
        material->baseColorFactor = color;

        lua_getfield(L, 1, "texture");
        if (!lua_isnil(L, -1)) {
            auto* texHandle = checkTextureHandle(L, lua_gettop(L), "gd3d.Material.new");
            std::uint64_t const texId = requireTextureId(L, texHandle, "gd3d.Material.new");
            auto texAsset = render3d::TextureRegistry::instance().get(texId);
            if (!texAsset) {
                luaL_error(L, "gd3d.Material.new: texture handle is invalid");
            }
            material->textureId = texId;
            material->texture = texAsset;
        }
        lua_pop(L, 1);

        pushMaterial(L, std::move(material));
        return 1;
    }

    void materialHandleDtor(lua_State*, void* ud) {
        static_cast<MaterialBox*>(ud)->~MaterialBox();
    }

    void registerMaterialMetatable(lua_State* L) {
        luaL_Reg const methods[] = {
            {"baseColor", materialBaseColor},
            {"hasTexture", materialHasTexture},
            {nullptr, nullptr},
        };

        registerTaggedMetatable(
            L, kMaterialMeta, detail::materialTag(), methods, std::nullopt, &materialHandleDtor, kMaterialTypeName
        );
    }
} // namespace

namespace luax {
    geode::Result<void> registerMaterial(lua_State* L) {
        registerMaterialMetatable(L);

        getOrCreateTable(L, "gd3d.Material");
        setTableCFunction(L, -1, "new", &materialNew);
        lua_pop(L, 1);

        return geode::Ok();
    }
} // namespace luax
