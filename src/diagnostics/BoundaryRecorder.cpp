#include "diagnostics/BoundaryRecorder.hpp"

#include "bindings/geode/CurrentMod.hpp"
#include "core/Runtime.hpp"
#include "core/StackFormat.hpp"

#include <Geode/loader/Dirs.hpp>
#include <Geode/loader/Loader.hpp>
#include <Geode/loader/Mod.hpp>
#include <Geode/utils/file.hpp>
#include <Geode/utils/string.hpp>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <lua.h>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace luax::diag {
    void flushIfNeeded(imes::luauapi::RuntimeStatus status, bool force);

    namespace {
        struct RecorderState {
            std::vector<BoundaryFrame> stack;
            std::filesystem::path crashlogsDir;
            bool dirty = false;
            std::chrono::steady_clock::time_point lastFlush{};
            bool lastFlushValid = false;
            std::string lastFlushKey;
            std::uint64_t flushEpoch = 0;
        };

        RecorderState& state() {
            static RecorderState s;
            return s;
        }

        bool& enabledFlag() {
            static bool enabled = true;
            return enabled;
        }

        std::string isoTimestampUtc() {
            auto now = std::chrono::system_clock::now();
            std::time_t t = std::chrono::system_clock::to_time_t(now);
            std::tm tm{};
#if defined(_WIN32)
            gmtime_s(&tm, &t);
#else
            gmtime_r(&t, &tm);
#endif
            return fmt::format(
                "{:04d}-{:02d}-{:02d}T{:02d}:{:02d}:{:02d}Z",
                tm.tm_year + 1900,
                tm.tm_mon + 1,
                tm.tm_mday,
                tm.tm_hour,
                tm.tm_min,
                tm.tm_sec
            );
        }

        char const* kindLabel(BoundaryKind kind) {
            switch (kind) {
                case BoundaryKind::Script: return "script";
                case BoundaryKind::HookTrampoline: return "hook-trampoline";
                case BoundaryKind::HookBefore: return "hook-before";
                case BoundaryKind::HookAfter: return "hook-after";
                case BoundaryKind::GeneratedBinding: return "generated-binding";
                case BoundaryKind::HandwrittenBinding: return "handwritten-binding";
                case BoundaryKind::Task: return "task";
                case BoundaryKind::ImGui: return "imgui";
                case BoundaryKind::Delegate: return "delegate";
            }
            return "unknown";
        }

        std::string frameSummary(BoundaryFrame const& f, std::size_t index) {
            std::string out = fmt::format("#{} {}  {}", index, kindLabel(f.kind), f.target);
            if (!f.modId.empty()) {
                fmt::format_to(std::back_inserter(out), "  mod={}", f.modId);
            }
            if (f.callbackId != 0) {
                fmt::format_to(std::back_inserter(out), "  cb={}", f.callbackId);
            }
            return out;
        }

        std::string flushKey(BoundaryFrame const& f) {
            return fmt::format("{}|{}|{}", static_cast<unsigned>(f.kind), f.target, f.callbackId);
        }

        std::filesystem::path resolveCrashlogsDir() {
            auto const& s = state();
            if (!s.crashlogsDir.empty()) return s.crashlogsDir;
            return geode::dirs::getCrashlogsDir();
        }

        void fillMod(BoundaryFrame& frame, geode::Mod* mod) {
            if (!mod) return;
            frame.modId = mod->getID();
            frame.modVersion = mod->getVersion().toVString();
        }

        void fillStack(BoundaryFrame& frame, lua_State* L, int pendingFuncIndex) {
            if (!L) return;

            std::filesystem::path root;
            if (!frame.resourcesRoot.empty()) {
                root = frame.resourcesRoot;
            }
            else {
                auto* mod = currentMod();
                root = mod ? mod->getResourcesDir() : std::filesystem::path{};
            }

            if (pendingFuncIndex != 0) {
                frame.luaStack = formatPendingCall(L, pendingFuncIndex, root);
            }
            else if (frame.luaStack.empty()) {
                frame.luaStack = formatLuaStack(L, root);
            }
        }

        void flushAfterChange(bool force) {
            auto* runtime = Runtime::getIfInitialized();
            if (!runtime) return;
            flushIfNeeded(runtime->status(), force);
        }
    } // namespace

    BoundaryScope::~BoundaryScope() {
        popIfPushed();
    }

    BoundaryScope::BoundaryScope(BoundaryScope&& other) noexcept : m_pushed(other.m_pushed) {
        other.m_pushed = false;
    }

    BoundaryScope& BoundaryScope::operator=(BoundaryScope&& other) noexcept {
        if (this != &other) {
            popIfPushed();
            m_pushed = other.m_pushed;
            other.m_pushed = false;
        }
        return *this;
    }

    void BoundaryScope::popIfPushed() {
        if (!m_pushed) return;
        m_pushed = false;

        auto& s = state();
        if (!s.stack.empty()) {
            s.stack.pop_back();
        }
        s.dirty = true;
        flushAfterChange(false);
    }

    bool recordingEnabled() {
        return enabledFlag();
    }

    void setRecordingEnabled(bool enabled) {
        enabledFlag() = enabled;
    }

    BoundaryScope pushBoundary(BoundaryFrame frame, lua_State* L, int pendingFuncIndex) {
        BoundaryScope scope;
        if (!recordingEnabled()) return scope;

        auto& s = state();

        frame.timestamp = isoTimestampUtc();

        auto* mod = currentMod();
        fillMod(frame, mod);
        frame.resourcesRoot =
            mod ? geode::utils::string::pathToString(mod->getResourcesDir()) : std::string{};

        fillStack(frame, L, pendingFuncIndex);

        s.stack.push_back(std::move(frame));
        if (s.stack.size() > kSidecarStackDepth) {
            s.stack.erase(s.stack.begin());
        }

        s.dirty = true;
        scope.m_pushed = true;
        flushAfterChange(false);
        return scope;
    }

    BoundaryScope recordBindingEntry(lua_State* L, std::string_view label, BoundaryKind kind) {
        if (!recordingEnabled()) return {};

        BoundaryFrame frame;
        frame.kind = kind;
        frame.target = std::string(label);
        auto scope = pushBoundary(std::move(frame), L, 0);
        refreshActiveBoundaryStack(L);
        flushAfterChange(true);
        return scope;
    }

    void refreshActiveBoundaryStack(lua_State* L) {
        if (!recordingEnabled() || !L) return;

        auto& s = state();
        if (s.stack.empty()) return;

        auto* mod = currentMod();
        std::filesystem::path root = mod ? mod->getResourcesDir() : std::filesystem::path{};
        if (!s.stack.back().resourcesRoot.empty()) {
            root = s.stack.back().resourcesRoot;
        }

        s.stack.back().luaStack = formatLuaStack(L, root);
        s.dirty = true;
    }

    BoundaryFrame const* activeBoundary() {
        auto const& s = state();
        return s.stack.empty() ? nullptr : &s.stack.back();
    }

    std::span<BoundaryFrame const> callChain() {
        auto const& s = state();
        return std::span<BoundaryFrame const>(s.stack.data(), s.stack.size());
    }

    void resetForTests() {
        auto& s = state();
        s.stack.clear();
        s.dirty = false;
        s.lastFlushValid = false;
        s.lastFlushKey.clear();
        s.flushEpoch = 0;
    }

    void setCrashlogsDirForTests(std::filesystem::path dir) {
        state().crashlogsDir = std::move(dir);
    }

    static std::string serialize(imes::luauapi::RuntimeStatus status) {
        auto const& s = state();

        auto statusText = [](imes::luauapi::RuntimeStatus st) -> char const* {
            switch (st) {
                case imes::luauapi::RuntimeStatus::NotReady: return "NotReady";
                case imes::luauapi::RuntimeStatus::Ready: return "Ready";
                case imes::luauapi::RuntimeStatus::InitFailed: return "InitFailed";
                case imes::luauapi::RuntimeStatus::Panicked: return "Panicked";
            }
            return "Unknown";
        };

        std::string out;
        auto append = [&](std::string_view line) {
            out.append(line);
            out.push_back('\n');
        };

        append("=== LuauAPI Context ===");
        append(fmt::format("timestamp: {}", isoTimestampUtc()));
        append(fmt::format("runtime_status: {}", statusText(status)));
        append(fmt::format("geode_version: {}", geode::Loader::get()->getVersion().toVString()));
        append(
            fmt::format(
                "luauapi_version: {}",
                geode::Mod::get() ? geode::Mod::get()->getVersion().toVString() :
                                    std::string("(unknown)")
            )
        );
        out.push_back('\n');

        append("=== Active boundary (innermost) ===");
        if (s.stack.empty()) {
            append("(none)");
        }
        else {
            auto const& f = s.stack.back();
            append(fmt::format("kind: {}", kindLabel(f.kind)));
            append(fmt::format("target: {}", f.target));
            if (f.kind == BoundaryKind::HookBefore || f.kind == BoundaryKind::HookAfter) {
                append(
                    fmt::format("hook_kind: {}", f.kind == BoundaryKind::HookBefore ? "before" : "after")
                );
                append(fmt::format("callback_id: {}", f.callbackId));
            }
            append(fmt::format("mod_id: {}", f.modId.empty() ? "(none)" : f.modId));
            append(fmt::format("mod_version: {}", f.modVersion.empty() ? "(none)" : f.modVersion));
            append(
                fmt::format("resources_root: {}", f.resourcesRoot.empty() ? "(none)" : f.resourcesRoot)
            );
        }
        out.push_back('\n');

        append("=== Luau stack ===");
        if (s.stack.empty() || !hasStackFrames(s.stack.back().luaStack)) {
            append("(no stack captured)");
        }
        else {
            std::string_view stack = s.stack.back().luaStack;
            std::size_t firstNl = stack.find('\n');
            std::string_view body = stack.substr(firstNl + 1);
            if (!body.empty()) {
                out.append(body);
                if (body.back() != '\n') out.push_back('\n');
            }
            else {
                append("(no stack captured)");
            }
        }
        out.push_back('\n');

        append("=== Call chain (innermost first) ===");
        if (s.stack.empty()) {
            append("(none)");
        }
        else {
            std::size_t index = 0;
            for (auto it = s.stack.rbegin(); it != s.stack.rend(); ++it, ++index) {
                append(frameSummary(*it, index));
            }
        }
        out.push_back('\n');

        append("=== Notes ===");
        append(
            "Recorded immediately before native code ran. C++ fault address is in the main "
            "crashlog."
        );

        return out;
    }

    void flushIfNeeded(imes::luauapi::RuntimeStatus status, bool force) {
        auto& s = state();
        if (!s.dirty) return;
        if (s.stack.empty()) return;

        auto now = std::chrono::steady_clock::now();
        bool const activeChanged = !s.lastFlushValid ||
            (s.stack.empty() ? !s.lastFlushKey.empty() : flushKey(s.stack.back()) != s.lastFlushKey);
        bool const intervalElapsed = !s.lastFlushValid ||
            std::chrono::duration_cast<std::chrono::milliseconds>(now - s.lastFlush).count() >=
                kSidecarFlushIntervalMs;

        if (!force && !activeChanged && !intervalElapsed) return;

        std::filesystem::path dir;
        try {
            dir = resolveCrashlogsDir();
        }
        catch (...) {
            return;
        }
        if (dir.empty()) return;

        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        if (ec) return;

        std::filesystem::path tmp = dir / kSidecarTempName;
        std::filesystem::path final = dir / kSidecarFileName;

        std::string payload = serialize(status);

        {
            std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
            if (!out.good()) return;
            out.write(payload.data(), static_cast<std::streamsize>(payload.size()));
            if (!out.good()) {
                std::error_code rmec;
                std::filesystem::remove(tmp, rmec);
                return;
            }
            out.flush();
        }

        std::filesystem::rename(tmp, final, ec);
        if (ec) {
            std::error_code copyEc;
            std::filesystem::copy_file(
                tmp, final, std::filesystem::copy_options::overwrite_existing, copyEc
            );
            if (copyEc) {
                std::error_code rmec;
                std::filesystem::remove(tmp, rmec);
                return;
            }
            std::filesystem::remove(tmp, ec);
        }

        s.dirty = false;
        s.lastFlush = now;
        s.lastFlushValid = true;
        s.lastFlushKey = s.stack.empty() ? std::string{} : flushKey(s.stack.back());
        s.flushEpoch += 1;
    }
} // namespace luax::diag
