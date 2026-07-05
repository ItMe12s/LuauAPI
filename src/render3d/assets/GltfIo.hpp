#pragma once

#include "render3d/assets/MeshAsset.hpp"

#include <Geode/Result.hpp>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct cgltf_options;
struct cgltf_image;

namespace luax::render3d {
    struct SandboxFileContext {
        std::filesystem::path sandboxRoot;
        std::string lastError;
    };

    geode::Result<std::filesystem::path> canonicalSandboxRoot(std::filesystem::path const& root);

    void configureSandboxFileIo(::cgltf_options& options, SandboxFileContext& context);

    geode::Result<std::vector<std::uint8_t>> readImageEncodedBytes(
        ::cgltf_image const* image, std::filesystem::path const& assetPath,
        std::filesystem::path const& sandboxRoot
    );

} // namespace luax::render3d
