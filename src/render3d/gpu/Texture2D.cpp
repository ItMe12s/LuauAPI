#include "render3d/gpu/Texture2D.hpp"

#include "render3d/assets/ImageDecode.hpp"
#include "render3d/gpu/GlUtil.hpp"

#include <Geode/Geode.hpp>

namespace luax::render3d {

    unsigned int uploadRgbaTexture2D(std::span<std::uint8_t const> rgba, int width, int height) {
        if (!glContextAvailable() || width <= 0 || height <= 0) {
            return 0;
        }
        if (rgba.size() != static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4) {
            return 0;
        }

        drainGlErrors();
        unsigned int texture = 0;
        glGenTextures(1, &texture);
        if (texture == 0) {
            return 0;
        }
        glBindTexture(GL_TEXTURE_2D, texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data()
        );
        if (glGetError() != GL_NO_ERROR) {
            geode::log::error("Renderer3D: texture upload failed ({}x{})", width, height);
            glDeleteTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, 0);
            return 0;
        }
        glBindTexture(GL_TEXTURE_2D, 0);
        return texture;
    }

    unsigned int uploadRgbaTexture2D(ImageData const& image) {
        if (image.width <= 0 || image.height <= 0 || image.rgba.empty()) {
            return 0;
        }
        return uploadRgbaTexture2D(image.rgba, image.width, image.height);
    }

    unsigned int allocFramebufferTexture(int width, int height) {
        if (!glContextAvailable() || width <= 0 || height <= 0) {
            return 0;
        }

        drainGlErrors();
        unsigned int texture = 0;
        glGenTextures(1, &texture);
        if (texture == 0) {
            return 0;
        }
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        if (glGetError() != GL_NO_ERROR) {
            geode::log::error("Renderer3D: framebuffer texture alloc failed ({}x{})", width, height);
            glDeleteTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, 0);
            return 0;
        }
        return texture;
    }

} // namespace luax::render3d