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

    void releaseTextureBox(TextureBox* box) {
#if !defined(LUAUAPI_HOST_TESTS)
        if (box != nullptr && box->texture) {
            Renderer3D::instance().releaseTextureGpu(box->texture.get());
        }
#endif
        if (box != nullptr) {
            box->texture.reset();
        }
    }

    int textureSizeAxis(lua_State* L, char const* method, bool wantWidth) {
        auto& texture = requireTexture(L, checkTextureHandle(L, 1, method), method);
        auto* viewport = texture->viewportSource();
        if (viewport != nullptr) {
            push(L, wantWidth ? viewport->framebufferPixelWidth() : viewport->framebufferPixelHeight());
            return 1;
        }
        push(L, wantWidth ? texture->cpu.width : texture->cpu.height);
        return 1;
    }

    int textureWidth(lua_State* L) {
        return textureSizeAxis(L, "Texture:width", true);
    }

    int textureHeight(lua_State* L) {
        return textureSizeAxis(L, "Texture:height", false);
    }

    int textureGc(lua_State* L) {
        releaseTextureBox(checkTextureHandle(L, 1, "Texture.__gc"));
        return 0;
    }

    void textureBoxDtor(lua_State* L, void* ud) {
        (void)L;
        releaseTextureBox(static_cast<TextureBox*>(ud));
        static_cast<TextureBox*>(ud)->~TextureBox();
    }

    void registerTextureHandleMetatable(lua_State* L) {
        luaL_Reg const methods[] = {
            {"width", textureWidth},
            {"height", textureHeight},
            {"__gc", textureGc},
            {nullptr, nullptr},
        };

        registerTaggedMetatable(
            L, kTextureMeta, luax::detail::textureTag(), methods, std::nullopt, &textureBoxDtor, kTextureTypeName
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
        if (auto err = returnIfErr(L, result)) {
            return *err;
        }

        auto asset = std::make_shared<TextureAsset>();
        asset->cpu = std::move(result.unwrap());
        pushTextureHandle(L, std::move(asset));
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
