#include "render3d/assets/GltfIo.hpp"

#include "core/Config.hpp"
#include "require/PathSandbox.hpp"

#include <Geode/utils/base64.hpp>
#include <Geode/utils/file.hpp>
#include <cstring>
#include <string_view>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

namespace {
    using namespace luax;
    using namespace luax::render3d;

    enum class PostReadValidation {
        ExactFileSize,
        MaximumSize,
    };

    geode::Result<std::vector<std::uint8_t>> readSandboxFile(
        std::filesystem::path const& path, std::filesystem::path const& sandboxRoot,
        std::string_view label, PostReadValidation validation
    ) {
        std::string const prefix(label);
        std::error_code ec;
        auto const rel = std::filesystem::relative(path, sandboxRoot, ec);
        if (ec) {
            return geode::Err(prefix + " path cannot be resolved: " + ec.message());
        }
        if (escapedRelativePathValue(rel)) {
            return geode::Err(prefix + " path escapes sandbox root");
        }
        GEODE_UNWRAP_INTO(auto resolved, resolveInsideRoot(sandboxRoot, rel.generic_string()));

        if (!std::filesystem::is_regular_file(resolved, ec)) {
            return geode::Err(prefix + " file not found: " + filesystemPathString(resolved));
        }

        auto const fileSize = std::filesystem::file_size(resolved, ec);
        if (ec) {
            return geode::Err(prefix + " file cannot be read: " + filesystemPathString(resolved));
        }

        if (fileSize > kMaxFsReadBytes) {
            return geode::Err(prefix + " file exceeds maximum read size");
        }

        auto bytesResult = geode::utils::file::readBinary(resolved);
        if (bytesResult.isErr()) {
            return geode::Err(prefix + " file cannot be read: " + filesystemPathString(resolved));
        }

        auto bytes = std::move(bytesResult).unwrap();
        if (validation == PostReadValidation::ExactFileSize && bytes.size() != fileSize) {
            return geode::Err(prefix + " file cannot be read: " + filesystemPathString(resolved));
        }
        if (validation == PostReadValidation::MaximumSize && bytes.size() > kMaxFsReadBytes) {
            return geode::Err(prefix + " file exceeds maximum read size");
        }

        return geode::Ok(std::move(bytes));
    }

    cgltf_result sandboxFileRead(
        cgltf_memory_options const* memory, cgltf_file_options const* file, char const* path,
        cgltf_size* size, void** data
    ) {
        void* (*memoryAlloc)(void*, cgltf_size) =
            memory->alloc_func != nullptr ? memory->alloc_func : &cgltf_default_alloc;

        auto* context = static_cast<SandboxFileContext*>(file->user_data);
        if (context == nullptr) {
            return cgltf_result_invalid_options;
        }

        auto read = readSandboxFile(
            std::filesystem::path(path), context->sandboxRoot, "buffer", PostReadValidation::ExactFileSize
        );
        if (read.isErr()) {
            context->lastError = std::move(read).unwrapErr();
            return cgltf_result_io_error;
        }

        auto bytes = std::move(read).unwrap();
        auto* buffer = static_cast<std::uint8_t*>(memoryAlloc(memory->user_data, bytes.size()));
        if (buffer == nullptr) {
            return cgltf_result_out_of_memory;
        }

        std::memcpy(buffer, bytes.data(), bytes.size());

        *size = bytes.size();
        *data = buffer;
        return cgltf_result_success;
    }

    void sandboxFileRelease(
        cgltf_memory_options const* memory, cgltf_file_options const* file, void* data
    ) {
        void (*memoryFree)(void*, void*) =
            memory->free_func != nullptr ? memory->free_func : &cgltf_default_free;
        memoryFree(memory->user_data, data);
        (void)file;
    }

    geode::Result<std::vector<std::uint8_t>> decodeBase64ToBytes(char const* base64) {
        if (base64 == nullptr) {
            return geode::Err("base64 data is missing");
        }

        auto decoded = geode::utils::base64::decode(
            std::string_view(base64), geode::utils::base64::Base64Variant::Normal
        );
        if (decoded.isErr()) {
            return geode::Err("invalid base64 image data");
        }

        auto bytes = std::move(decoded.unwrap());
        if (bytes.empty()) {
            return geode::Err("base64 image data is empty");
        }

        if (bytes.size() > kMaxFsReadBytes) {
            return geode::Err("base64 image data exceeds maximum read size");
        }

        return geode::Ok(bytes);
    }

} // namespace

namespace luax::render3d {

    void configureSandboxFileIo(::cgltf_options& options, SandboxFileContext& context) {
        options.file.read = sandboxFileRead;
        options.file.release = sandboxFileRelease;
        options.file.user_data = &context;
    }

    geode::Result<std::vector<std::uint8_t>> readImageEncodedBytes(
        ::cgltf_image const* image, std::filesystem::path const& assetPath,
        std::filesystem::path const& sandboxRoot
    ) {
        if (image == nullptr) {
            return geode::Err("image is missing");
        }

        if (image->buffer_view != nullptr) {
            cgltf_buffer_view const* view = image->buffer_view;
            if (view->buffer == nullptr || view->buffer->data == nullptr) {
                return geode::Err("embedded image buffer has no data");
            }

            if (view->offset > view->buffer->size || view->size > view->buffer->size - view->offset) {
                return geode::Err("embedded image buffer view is out of range");
            }

            if (view->size > kMaxFsReadBytes) {
                return geode::Err("embedded image exceeds maximum read size");
            }

            auto const* data = static_cast<std::uint8_t const*>(view->buffer->data) + view->offset;
            std::vector<std::uint8_t> bytes(static_cast<std::size_t>(view->size));
            std::memcpy(bytes.data(), data, static_cast<std::size_t>(view->size));
            return geode::Ok(bytes);
        }

        if (image->uri == nullptr || image->uri[0] == '\0') {
            return geode::Err("image has no uri or buffer_view");
        }

        if (std::strncmp(image->uri, "data:", 5) == 0) {
            char const* comma = std::strchr(image->uri, ',');
            if (comma == nullptr || comma - image->uri < 7 ||
                std::strncmp(comma - 7, ";base64", 7) != 0) {
                return geode::Err("unsupported image data uri");
            }

            return decodeBase64ToBytes(comma + 1);
        }

        return readSandboxFile(
            assetPath.parent_path() / image->uri, sandboxRoot, "image", PostReadValidation::MaximumSize
        );
    }

} // namespace luax::render3d