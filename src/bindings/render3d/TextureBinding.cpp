#include "bindings/geode/ModSandbox.hpp"
#include "bindings/render3d/Gd3dShared.hpp"
#include "core/Config.hpp"
#include "framework/Binding.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"
#include "framework/stack/UserdataTags.hpp"
#include "render3d/viewport/CCViewportFrame.hpp"
#include "render3d/assets/ImageDecode.hpp"
#include "render3d/gpu/Renderer3D.hpp"
#include "render3d/assets/TextureAsset.hpp"

#include <Geode/Geode.hpp>
#include <Geode/utils/file.hpp>
#include <filesystem>
#include <lua.h>
#include <lualib.h>
#include <memory>
#include <span>

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

        if (luaL_newmetatable(L, kTextureMeta)) {
            for (luaL_Reg const* reg = methods; reg->name != nullptr; ++reg) {
                setTableCFunction(L, -1, reg->name, reg->func);
            }
            lua_pushvalue(L, -1);
            lua_setfield(L, -2, "__index");
            lua_pushstring(L, "locked");
            lua_setfield(L, -2, "__metatable");
            lua_pushstring(L, kTextureTypeName);
            lua_setfield(L, -2, "__type");
        }
        lua_pop(L, 1);

        lua_getuserdatametatable(L, luax::detail::textureTag());
        if (!lua_isnil(L, -1)) {
            lua_pop(L, 1);
            return;
        }
        lua_pop(L, 1);

        luaL_getmetatable(L, kTextureMeta);
        lua_setuserdatametatable(L, luax::detail::textureTag());
        lua_setuserdatadtor(L, luax::detail::textureTag(), &textureHandleDtor);
    }

    int textureLoad(lua_State* L) {
        auto target = resolveSandboxTarget(L, 1, 2, "gd3d.texture.load");
        if (!target) {
            return 2;
        }

        std::error_code ec;
        if (!std::filesystem::is_regular_file(target->path, ec)) {
            return pushNilErr(L, "path is not a regular file");
        }

        auto contents = geode::utils::file::readString(target->path);
        if (contents.isErr()) {
            return pushNilErr(L, contents.unwrapErr());
        }
        auto const& data = contents.unwrap();
        if (data.size() > kMaxFsReadBytes) {
            return pushNilErr(L, "file exceeds maximum read size");
        }

        auto const bytes = std::span<std::uint8_t const>(
            reinterpret_cast<std::uint8_t const*>(data.data()), data.size()
        );
        auto result = decodeImageRgba8(bytes);
        if (result.isErr()) {
            return pushNilErr(L, result.unwrapErr());
        }

        auto asset = std::make_shared<TextureAsset>();
        asset->cpu = std::move(result.unwrap());
        auto const id = TextureRegistry::instance().registerTexture(asset);
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

#if !defined(LUAUAPI_HOST_TESTS)
LUAX_BINDING(gd3d_texture_lib, registerTexture)
#endif
