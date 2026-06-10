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
    constexpr std::uint32_t kFirstDynamicUsertypeTag = 7;

    static_assert(
        kFirstDynamicUsertypeTag < LUA_UTAG_LIMIT, "Reserved userdata tags exceed LUA_UTAG_LIMIT"
    );
    static_assert(
        kOpaqueHandleUserdataTag < kTaskHandleUserdataTag &&
            kTaskHandleUserdataTag < kImGuiDrawHandleUserdataTag &&
            kImGuiDrawHandleUserdataTag < kWsConnectionUserdataTag &&
            kWsConnectionUserdataTag < kWsServerUserdataTag &&
            kWsServerUserdataTag < kWsPeerUserdataTag &&
            kWsPeerUserdataTag < kFirstDynamicUsertypeTag,
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

    constexpr bool isReservedUserdataTag(std::uint32_t tag) noexcept {
        return tag >= kOpaqueHandleUserdataTag && tag < kFirstDynamicUsertypeTag;
    }
} // namespace luax::detail
