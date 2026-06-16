#include "render3d/gpu/Texture2D.hpp"

#include "render3d/assets/MeshAsset.hpp"
#include "render3d/gpu/GlUtil.hpp"

namespace luax::render3d {

    namespace {
        GLenum wrapModeGl(TextureWrapMode wrap) {
            return wrap == TextureWrapMode::ClampToEdge ? GL_CLAMP_TO_EDGE : GL_REPEAT;
        }
    } // namespace

    unsigned int uploadRgbaTexture2D(
        int width, int height, std::uint8_t const* rgba, TextureWrapMode wrap, bool leaveBound
    ) {
        if (!glContextAvailable() || width <= 0 || height <= 0) {
            return 0;
        }

        unsigned int texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GLenum const wrapGl = wrapModeGl(wrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapGl);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapGl);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
        if (!leaveBound) {
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        return texture;
    }

    unsigned int uploadRgbaTexture2D(ImageData const& image, TextureWrapMode wrap, bool leaveBound) {
        if (image.width <= 0 || image.height <= 0 || image.rgba.empty()) {
            return 0;
        }
        return uploadRgbaTexture2D(image.width, image.height, image.rgba.data(), wrap, leaveBound);
    }

} // namespace luax::render3d
