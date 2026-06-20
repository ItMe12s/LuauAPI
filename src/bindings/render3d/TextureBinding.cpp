#include "bindings/geode/ModSandbox.hpp"
#include "bindings/render3d/internal/Handles.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"
#include "framework/stack/TaggedMetatable.hpp"
#include "framework/stack/UserdataTags.hpp"
#include "render3d/assets/ImageDecode.hpp"
#include "render3d/assets/TextureAsset.hpp"
#include "render3d/gpu/Renderer3D.hpp"
#include "render3d/viewport/CCViewportFrame.hpp"

#include <Geode/Geode.hpp>
#include <lua.h>
#include <lualib.h>
#include <memory>

namespace {
    using namespace luax;
    using namespace luax::gd3d;
    using namespace luax::render3d;

    void releaseTextureHandle(TextureHandle* handle) {
        if (handle == nullptr || handle->id == 0) {
            return;
        }
        std::uint64_t const id = handle->id;
        TextureRegistry::instance().release(id);
        Renderer3D::instance().releaseTextureGpu(id);
        handle->id = 0;
    }

    int textureWidth(lua_State* L) {
        auto* handle = checkTextureHandle(L, 1, "Texture:width");
        auto texture = TextureRegistry::instance().get(requireTextureId(L, handle, "Texture:width"));
        if (!texture) {
            luaL_error(L, "Texture:width: texture handle is invalid");
        }
        auto* viewport = texture->viewportSource();
        if (viewport != nullptr) {
            push(L, viewport->framebufferPixelWidth());
            return 1;
        }
        push(L, texture->cpu.width);
        return 1;
    }

    int textureHeight(lua_State* L) {
        auto* handle = checkTextureHandle(L, 1, "Texture:height");
        auto texture = TextureRegistry::instance().get(requireTextureId(L, handle, "Texture:height"));
        if (!texture) {
            luaL_error(L, "Texture:height: texture handle is invalid");
        }
        auto* viewport = texture->viewportSource();
        if (viewport != nullptr) {
            push(L, viewport->framebufferPixelHeight());
            return 1;
        }
        push(L, texture->cpu.height);
        return 1;
    }

    int textureGc(lua_State* L) {
        releaseTextureHandle(checkTextureHandle(L, 1, "Texture.__gc"));
        return 0;
    }

    void textureHandleDtor(lua_State* L, void* ud) {
        (void)L;
        releaseTextureHandle(static_cast<TextureHandle*>(ud));
    }

    void registerTextureHandleMetatable(lua_State* L) {
        luaL_Reg const methods[] = {
            {"width", textureWidth},
            {"height", textureHeight},
            {"__gc", textureGc},
            {nullptr, nullptr},
        };

        registerTaggedMetatable(
            L, kTextureMeta, luax::detail::textureTag(), methods, std::nullopt, &textureHandleDtor, kTextureTypeName
        );
    }

    int textureLoad(lua_State* L) {
        auto target = resolveSandboxTarget(L, 1, 2, "gd3d.texture.load");
        if (!target) {
            return 2;
        }

        auto contents = readSandboxBinaryFile(target->path);
        if (contents.isErr()) {
            return pushNilErr(L, contents.unwrapErr());
        }
        auto const& bytes = contents.unwrap();

        auto result = decodeImageRgba8(bytes);
        if (!result.has_value()) {
            return pushNilErr(L, result.error());
        }

        auto asset = std::make_shared<TextureAsset>();
        asset->cpu = std::move(result).value();
        auto const id = TextureRegistry::instance().registerAsset(asset);
        pushTextureHandle(L, id);
        return 1;
    }
} // namespace

namespace luax {
    geode::Result<void> registerTexture(lua_State* L) {
        registerTextureHandleMetatable(L);

        getOrCreateTable(L, "gd3d.texture");
        setTableCFunction(L, -1, "load", &textureLoad);
        lua_pop(L, 1);

        return geode::Ok();
    }
} // namespace luax
