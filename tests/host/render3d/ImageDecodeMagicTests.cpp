#include "render3d/assets/ImageDecode.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>

TEST_CASE("isJpegMagic detects JPEG SOI marker") {
    std::array<std::uint8_t const, 4> const jpeg = {0xFF, 0xD8, 0xFF, 0xE0};
    CHECK(luax::render3d::isJpegMagic(jpeg));

    std::array<std::uint8_t const, 8> const png = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    CHECK_FALSE(luax::render3d::isJpegMagic(png));

    std::array<std::uint8_t const, 2> const tiny = {0xFF, 0xD8};
    CHECK_FALSE(luax::render3d::isJpegMagic(tiny));
}

TEST_CASE("host decode stub fails closed with a clear error") {
    std::array<std::uint8_t const, 8> const png = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    auto result = luax::render3d::decodeImageRgba8(png);
    REQUIRE(result.isErr());
    REQUIRE(result.unwrapErr() == "image decode is not available on host");
}
