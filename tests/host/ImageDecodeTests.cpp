#include "core/Config.hpp"
#include "render3d/assets/ImageDecode.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <span>
#include <vector>

namespace {
    using namespace luax;
    using namespace luax::render3d;

    std::array<std::uint8_t, 68> const k1x1Png = {
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48,
        0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x06, 0x00, 0x00,
        0x00, 0x1F, 0x15, 0xC4, 0x89, 0x00, 0x00, 0x00, 0x0A, 0x49, 0x44, 0x41, 0x54, 0x78,
        0x9C, 0x63, 0x00, 0x01, 0x00, 0x00, 0x05, 0x00, 0x01, 0x0D, 0x0A, 0x2D, 0xB4, 0x00,
        0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82,
    };
} // namespace

TEST_CASE("decodeImageRgba8 decodes embedded 1x1 PNG") {
    auto result = decodeImageRgba8(std::span<std::uint8_t const>(k1x1Png.data(), k1x1Png.size()));
    REQUIRE(result.has_value());

    auto const& image = result.value();
    REQUIRE(image.width == 1);
    REQUIRE(image.height == 1);
    REQUIRE(image.rgba.size() == 4);
}

TEST_CASE("decodeImageRgba8 rejects empty input") {
    std::vector<std::uint8_t> const empty;
    auto result = decodeImageRgba8(std::span<std::uint8_t const>(empty.data(), empty.size()));
    REQUIRE(!result.has_value());
    REQUIRE(result.error() == "image data is empty");
}

TEST_CASE("decodeImageRgba8 rejects invalid bytes") {
    std::vector<std::uint8_t> const invalid{0x00, 0x01, 0x02, 0x03};
    auto result = decodeImageRgba8(std::span<std::uint8_t const>(invalid.data(), invalid.size()));
    REQUIRE(!result.has_value());
    REQUIRE(result.error().starts_with("failed to decode image:"));
}

TEST_CASE("decodeImageRgba8 rejects encoded input above read cap") {
    std::vector<std::uint8_t> oversized(kMaxFsReadBytes + 1, 0xFF);
    auto result = decodeImageRgba8(std::span<std::uint8_t const>(oversized.data(), oversized.size()));
    REQUIRE(!result.has_value());
    REQUIRE(result.error() == "encoded image exceeds maximum read size");
}
