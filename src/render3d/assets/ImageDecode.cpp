#include "render3d/assets/ImageDecode.hpp"

#include "core/Config.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace luax::render3d {

    bool isJpegMagic(std::span<std::uint8_t const> bytes) {
        return bytes.size() >= 3 && bytes[0] == 0xFF && bytes[1] == 0xD8 && bytes[2] == 0xFF;
    }

} // namespace luax::render3d

#if defined(LUAUAPI_HOST_TESTS)
namespace luax::render3d {

    geode::Result<ImageData> decodeImageRgba8(std::span<std::uint8_t const>) {
        return geode::Err("image decode is not available on host");
    }

} // namespace luax::render3d
#else
    #include <Geode/Geode.hpp>
    #include <algorithm>
    #include <cocos2d.h>
    #include <prevter.imageplus/include/events.hpp>
    #include <string>
    #include <variant>

namespace luax::render3d {
    namespace {
        constexpr int kMaxImageDimension = 4096;

        geode::Result<ImageData> decodeJpegViaCocos(std::span<std::uint8_t const> encodedBytes) {
            cocos2d::CCImage image;
            if (!image.initWithImageData(
                    const_cast<std::uint8_t*>(encodedBytes.data()),
                    static_cast<int>(encodedBytes.size()),
                    cocos2d::CCImage::kFmtJpg
                )) {
                return geode::Err("failed to decode image: invalid JPEG data");
            }

            int const width = image.getWidth();
            int const height = image.getHeight();
            if (width <= 0 || height <= 0 || width > kMaxImageDimension ||
                height > kMaxImageDimension) {
                return geode::Err("decoded image has invalid dimensions");
            }

            unsigned char const* pixels = image.getData();
            if (pixels == nullptr) {
                return geode::Err("decoded image has invalid dimensions");
            }

            std::size_t const pixels64 =
                static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
            std::size_t const byteCount = pixels64 * 4;
            if (byteCount > kMaxFsReadBytes) {
                return geode::Err("decoded image exceeds maximum size");
            }

            ImageData out;
            out.width = width;
            out.height = height;
            out.rgba.resize(byteCount);
            // cocos JPEG path emits RGB, premult flag stays false.
            for (std::size_t i = 0; i < pixels64; ++i) {
                out.rgba[i * 4 + 0] = pixels[i * 3 + 0];
                out.rgba[i * 4 + 1] = pixels[i * 3 + 1];
                out.rgba[i * 4 + 2] = pixels[i * 3 + 2];
                out.rgba[i * 4 + 3] = 255;
            }
            return geode::Ok(std::move(out));
        }
    } // namespace

    geode::Result<ImageData> decodeImageRgba8(std::span<std::uint8_t const> encodedBytes) {
        if (encodedBytes.empty()) {
            return geode::Err("image data is empty");
        }

        if (encodedBytes.size() > kMaxFsReadBytes) {
            return geode::Err("encoded image exceeds maximum read size");
        }

        if (isJpegMagic(encodedBytes)) {
            return decodeJpegViaCocos(encodedBytes);
        }

        auto decoded = imgp::tryDecode(encodedBytes.data(), encodedBytes.size());
        if (decoded.isErr()) {
            return geode::Err(std::string("failed to decode image: ") + decoded.unwrapErr());
        }

        auto& result = decoded.unwrap();
        if (!std::holds_alternative<imgp::DecodedImage>(result)) {
            return geode::Err("animated images are not supported, use a static image");
        }

        auto& still = std::get<imgp::DecodedImage>(result);
        int const width = still.width;
        int const height = still.height;
        if (still.data == nullptr || width <= 0 || height <= 0 || width > kMaxImageDimension ||
            height > kMaxImageDimension) {
            return geode::Err("decoded image has invalid dimensions");
        }

        std::size_t const pixels = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
        std::size_t const byteCount = pixels * 4;
        if (byteCount > kMaxFsReadBytes) {
            return geode::Err("decoded image exceeds maximum size");
        }

        ImageData image;
        image.width = width;
        image.height = height;
        image.rgba.resize(byteCount);
        if (still.hasAlpha) {
            std::size_t const srcBytes = pixels * 4;
            std::copy_n(still.data.get(), srcBytes, image.rgba.data());
        }
        else {
            std::uint8_t const* src = still.data.get();
            for (std::size_t i = 0; i < pixels; ++i) {
                image.rgba[i * 4 + 0] = src[i * 3 + 0];
                image.rgba[i * 4 + 1] = src[i * 3 + 1];
                image.rgba[i * 4 + 2] = src[i * 3 + 2];
                image.rgba[i * 4 + 3] = 255;
            }
        }
        return geode::Ok(std::move(image));
    }

} // namespace luax::render3d
#endif
