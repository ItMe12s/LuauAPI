#pragma once

#include <cstdint>
#include <lua.h>

namespace luax::detail {
    constexpr std::uint32_t kOpaqueHandleUserdataTag = 1;
    constexpr std::uint32_t kTaskHandleUserdataTag = 2;
    constexpr std::uint32_t kImGuiDrawHandleUserdataTag = 3;
    constexpr std::uint32_t kWsConnectionUserdataTag = 4;
    constexpr std::uint32_t kWsServerUserdataTag = 5;
    constexpr std::uint32_t kWsPeerUserdataTag = 6;
    constexpr std::uint32_t kTransformUserdataTag = 7;
    constexpr std::uint32_t kMeshAssetUserdataTag = 8;
    constexpr std::uint32_t kMaterialUserdataTag = 9;
    constexpr std::uint32_t kTextureUserdataTag = 10;
    constexpr std::uint32_t kFirstDynamicUsertypeTag = 11;

    static_assert(
        kFirstDynamicUsertypeTag < LUA_UTAG_LIMIT, "Reserved userdata tags exceed LUA_UTAG_LIMIT"
    );
    static_assert(
        kOpaqueHandleUserdataTag < kTaskHandleUserdataTag &&
            kTaskHandleUserdataTag < kImGuiDrawHandleUserdataTag &&
            kImGuiDrawHandleUserdataTag < kWsConnectionUserdataTag &&
            kWsConnectionUserdataTag < kWsServerUserdataTag &&
            kWsServerUserdataTag < kWsPeerUserdataTag && kWsPeerUserdataTag < kTransformUserdataTag &&
            kTransformUserdataTag < kMeshAssetUserdataTag &&
            kMeshAssetUserdataTag < kMaterialUserdataTag &&
            kMaterialUserdataTag < kTextureUserdataTag &&
            kTextureUserdataTag < kFirstDynamicUsertypeTag,
        "Reserved userdata tags must be unique and ordered"
    );

    constexpr int opaqueHandleTag() noexcept {
        return static_cast<int>(kOpaqueHandleUserdataTag);
    }

    constexpr int taskHandleTag() noexcept {
        return static_cast<int>(kTaskHandleUserdataTag);
    }

    constexpr int imguiDrawHandleTag() noexcept {
        return static_cast<int>(kImGuiDrawHandleUserdataTag);
    }

    constexpr int wsConnectionTag() noexcept {
        return static_cast<int>(kWsConnectionUserdataTag);
    }

    constexpr int wsServerTag() noexcept {
        return static_cast<int>(kWsServerUserdataTag);
    }

    constexpr int wsPeerTag() noexcept {
        return static_cast<int>(kWsPeerUserdataTag);
    }

    constexpr int transformTag() noexcept {
        return static_cast<int>(kTransformUserdataTag);
    }

    constexpr int meshAssetTag() noexcept {
        return static_cast<int>(kMeshAssetUserdataTag);
    }

    constexpr int materialTag() noexcept {
        return static_cast<int>(kMaterialUserdataTag);
    }

    constexpr int textureTag() noexcept {
        return static_cast<int>(kTextureUserdataTag);
    }

    constexpr bool isReservedUserdataTag(std::uint32_t tag) noexcept {
        return tag >= kOpaqueHandleUserdataTag && tag < kFirstDynamicUsertypeTag;
    }
} // namespace luax::detail
