#include "render3d/assets/ImageDecode.hpp"

#include "core/Config.hpp"

#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_MAX_DIMENSIONS 8192
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace luax::render3d {

    std::expected<ImageData, std::string> decodeImageRgba8(std::span<std::uint8_t const> encodedBytes) {
        if (encodedBytes.empty()) {
            return std::unexpected("image data is empty");
        }

        if (encodedBytes.size() > kMaxFsReadBytes) {
            return std::unexpected("encoded image exceeds maximum read size");
        }

        stbi_set_flip_vertically_on_load(0);

        int width = 0;
        int height = 0;
        int channels = 0;
        unsigned char* pixels = stbi_load_from_memory(
            encodedBytes.data(), static_cast<int>(encodedBytes.size()), &width, &height, &channels, 4
        );
        if (pixels == nullptr) {
            char const* reason = stbi_failure_reason();
            return std::unexpected(
                std::string("failed to decode image: ") +
                (reason != nullptr ? reason : "unknown error")
            );
        }

        if (width <= 0 || height <= 0) {
            stbi_image_free(pixels);
            return std::unexpected("decoded image has invalid dimensions");
        }

        std::size_t const byteCount =
            static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4;
        if (byteCount > kMaxFsReadBytes) {
            stbi_image_free(pixels);
            return std::unexpected("decoded image exceeds maximum size");
        }

        ImageData image;
        image.width = width;
        image.height = height;
        image.rgba.assign(pixels, pixels + byteCount);
        stbi_image_free(pixels);
        return image;
    }

} // namespace luax::render3d
