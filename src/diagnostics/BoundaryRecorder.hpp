#pragma once

#include "core/Config.hpp"

#include <RuntimeTypes.hpp>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>

struct lua_State;

namespace geode {
    class Mod;
}

namespace luax::diag {
    enum class BoundaryKind : std::uint8_t {
        Script,
        HookTrampoline,
        HookBefore,
        HookAfter,
        GeneratedBinding,
        HandwrittenBinding,
        Task,
        ImGui,
        Delegate,
    };

    struct BoundaryFrame {
        BoundaryKind kind = BoundaryKind::Script;
        std::string target;
        std::uint64_t callbackId = 0;
        std::string modId;
        std::string modVersion;
        std::string resourcesRoot;
        std::string luaStack;
        std::string timestamp;
    };

    struct ProtectedCallBoundary {
        BoundaryKind kind = BoundaryKind::Script;
        std::uint64_t callbackId = 0;
        bool record = true;
    };

    class BoundaryScope {
    public:
        BoundaryScope() = default;
        ~BoundaryScope();
        BoundaryScope(BoundaryScope&& other) noexcept;
        BoundaryScope& operator=(BoundaryScope&& other) noexcept;

        BoundaryScope(BoundaryScope const&) = delete;
        BoundaryScope& operator=(BoundaryScope const&) = delete;

        explicit operator bool() const {
            return m_pushed;
        }

    private:
        friend BoundaryScope pushBoundary(
            BoundaryFrame frame, lua_State* L, int pendingFuncIndex, bool flushOnPush
        );

        void popIfPushed();

        bool m_pushed = false;
    };

    bool recordingEnabled();
    void setRecordingEnabled(bool enabled);

    BoundaryScope pushBoundary(
        BoundaryFrame frame, lua_State* L, int pendingFuncIndex = 0, bool flushOnPush = true
    );
    BoundaryScope recordBindingEntry(lua_State* L, std::string_view label, BoundaryKind kind);

    void refreshActiveBoundaryStack(lua_State* L);

    BoundaryFrame const* activeBoundary();
    std::span<BoundaryFrame const> callChain();

    void flushIfNeeded(imes::luauapi::RuntimeStatus status, bool force = false);

    void resetForTests();
    void setCrashlogsDirForTests(std::filesystem::path dir);

#if defined(LUAUAPI_HOST_TESTS)
    std::uint64_t flushEpochForTests();
#endif
} // namespace luax::diag
