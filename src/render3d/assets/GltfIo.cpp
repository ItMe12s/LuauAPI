#include "render3d/assets/GltfIo.hpp"

#include "core/Config.hpp"
#include "require/PathRules.hpp"
#include "require/PathSandbox.hpp"

#include <Geode/utils/base64.hpp>
#include <Geode/utils/file.hpp>
#include <cstring>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

namespace {
    using namespace luax;
    using namespace luax::render3d;

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

        std::error_code ec;
        auto resolved = std::filesystem::weakly_canonical(std::filesystem::path(path), ec);
        if (ec) {
            context->lastError = "buffer path cannot be resolved: " + ec.message();
            return cgltf_result_io_error;
        }

        if (!pathInsideRootValue(resolved, context->sandboxRoot)) {
            context->lastError = "buffer path escapes sandbox root";
            return cgltf_result_io_error;
        }

        if (!std::filesystem::is_regular_file(resolved, ec)) {
            context->lastError = "buffer file not found: " + filesystemPathString(resolved);
            return cgltf_result_file_not_found;
        }

        auto fileSize = std::filesystem::file_size(resolved, ec);
        if (ec) {
            context->lastError = "buffer file cannot be read: " + filesystemPathString(resolved);
            return cgltf_result_io_error;
        }

        if (fileSize > kMaxFsReadBytes) {
            context->lastError = "buffer file exceeds maximum read size";
            return cgltf_result_io_error;
        }

        auto bytesResult = geode::utils::file::readBinary(resolved);
        if (bytesResult.isErr()) {
            context->lastError = "buffer file cannot be read: " + filesystemPathString(resolved);
            return cgltf_result_io_error;
        }

        auto const& bytes = bytesResult.unwrap();
        if (bytes.size() != static_cast<std::size_t>(fileSize)) {
            context->lastError = "buffer file cannot be read: " + filesystemPathString(resolved);
            return cgltf_result_io_error;
        }

        auto* buffer = static_cast<std::uint8_t*>(memoryAlloc(memory->user_data, fileSize));
        if (buffer == nullptr) {
            return cgltf_result_out_of_memory;
        }

        std::memcpy(buffer, bytes.data(), bytes.size());

        *size = fileSize;
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

    std::expected<std::vector<std::uint8_t>, std::string> decodeBase64ToBytes(char const* base64) {
        if (base64 == nullptr) {
            return std::unexpected("base64 data is missing");
        }

        auto decoded = geode::utils::base64::decode(
            std::string_view(base64), geode::utils::base64::Base64Variant::Normal
        );
        if (decoded.isErr()) {
            return std::unexpected("invalid base64 image data");
        }

        auto bytes = std::move(decoded.unwrap());
        if (bytes.empty()) {
            return std::unexpected("base64 image data is empty");
        }

        if (bytes.size() > kMaxFsReadBytes) {
            return std::unexpected("base64 image data exceeds maximum read size");
        }

        return bytes;
    }

    std::expected<std::vector<std::uint8_t>, std::string> readSandboxImageFile(
        std::filesystem::path const& path, std::filesystem::path const& sandboxRoot
    ) {
        std::error_code ec;
        auto resolved = std::filesystem::weakly_canonical(path, ec);
        if (ec) {
            return std::unexpected("image path cannot be resolved: " + ec.message());
        }

        if (!pathInsideRootValue(resolved, sandboxRoot)) {
            return std::unexpected("image path escapes sandbox root");
        }

        if (!std::filesystem::is_regular_file(resolved, ec)) {
            return std::unexpected("image file not found: " + filesystemPathString(resolved));
        }

        auto fileSize = std::filesystem::file_size(resolved, ec);
        if (ec) {
            return std::unexpected("image file cannot be read: " + filesystemPathString(resolved));
        }

        if (fileSize > kMaxFsReadBytes) {
            return std::unexpected("image file exceeds maximum read size");
        }

        auto bytesResult = geode::utils::file::readBinary(resolved);
        if (bytesResult.isErr()) {
            return std::unexpected("image file cannot be read: " + filesystemPathString(resolved));
        }

        auto bytes = std::move(bytesResult.unwrap());
        if (bytes.size() > kMaxFsReadBytes) {
            return std::unexpected("image file exceeds maximum read size");
        }

        return bytes;
    }
} // namespace

namespace luax::render3d {

    std::expected<std::filesystem::path, std::string> canonicalSandboxRoot(
        std::filesystem::path const& root
    ) {
        if (root.empty()) {
            return std::filesystem::path{};
        }

        std::error_code ec;
        auto canonical = std::filesystem::weakly_canonical(root, ec);
        if (ec) {
            return std::unexpected("sandbox root cannot be resolved: " + ec.message());
        }

        return canonical;
    }

    void configureSandboxFileIo(::cgltf_options& options, SandboxFileContext& context) {
        options.file.read = sandboxFileRead;
        options.file.release = sandboxFileRelease;
        options.file.user_data = &context;
    }

    std::expected<std::vector<std::uint8_t>, std::string> readImageEncodedBytes(
        ::cgltf_image const* image, std::filesystem::path const& assetPath,
        std::filesystem::path const& sandboxRoot
    ) {
        if (image == nullptr) {
            return std::unexpected("image is missing");
        }

        if (image->buffer_view != nullptr) {
            cgltf_buffer_view const* view = image->buffer_view;
            if (view->buffer == nullptr || view->buffer->data == nullptr) {
                return std::unexpected("embedded image buffer has no data");
            }

            if (view->offset > view->buffer->size || view->size > view->buffer->size - view->offset) {
                return std::unexpected("embedded image buffer view is out of range");
            }

            if (view->size > kMaxFsReadBytes) {
                return std::unexpected("embedded image exceeds maximum read size");
            }

            auto const* data = static_cast<std::uint8_t const*>(view->buffer->data) + view->offset;
            std::vector<std::uint8_t> bytes(static_cast<std::size_t>(view->size));
            std::memcpy(bytes.data(), data, static_cast<std::size_t>(view->size));
            return bytes;
        }

        if (image->uri == nullptr || image->uri[0] == '\0') {
            return std::unexpected("image has no uri or buffer_view");
        }

        if (std::strncmp(image->uri, "data:", 5) == 0) {
            char const* comma = std::strchr(image->uri, ',');
            if (comma == nullptr || comma - image->uri < 7 ||
                std::strncmp(comma - 7, ";base64", 7) != 0) {
                return std::unexpected("unsupported image data uri");
            }

            return decodeBase64ToBytes(comma + 1);
        }

        return readSandboxImageFile(assetPath.parent_path() / image->uri, sandboxRoot);
    }

} // namespace luax::render3d
