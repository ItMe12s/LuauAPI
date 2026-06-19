#include "bindings/imgui/ImGuiFontRegistry.hpp"

#include "bindings/imgui/ImGuiHost.hpp"

#include <Geode/Geode.hpp>
#include <algorithm>
#include <imgui.h>
#include <vector>

namespace luax {
    namespace {
        struct FontEntry {
            std::uint64_t id = 0;
            std::vector<std::uint8_t> data;
            float size = 0.f;
            ImFont* font = nullptr;
        };

        std::vector<FontEntry> s_entries;
        std::uint64_t s_nextId = 1;
    } // namespace

    std::uint64_t imguiFontAdd(float size, std::vector<std::uint8_t> data) {
        FontEntry entry;
        entry.id = s_nextId++;
        entry.data = std::move(data);
        entry.size = size;
        s_entries.push_back(std::move(entry));
        return s_entries.back().id;
    }

    ImFont* imguiFontResolve(std::uint64_t id) {
        if (id == 0) {
            return nullptr;
        }
        for (auto const& entry : s_entries) {
            if (entry.id == id) {
                return entry.font;
            }
        }
        return nullptr;
    }

    void imguiFontRemove(std::uint64_t id) {
        s_entries.erase(
            std::remove_if(
                s_entries.begin(),
                s_entries.end(),
                [id](FontEntry const& entry) {
                    return entry.id == id;
                }
            ),
            s_entries.end()
        );
    }

    void imguiFontRebuildAtlas() {
        if (ImGui::GetCurrentContext() == nullptr) {
            return;
        }

        float const density = geode::utils::getDisplayFactor();
        if (s_entries.empty() && density <= 1.f) {
            return;
        }

        ImFontConfig cfg;
        if (density > 1.f) {
            cfg.RasterizerDensity = density;
        }

        auto& io = ImGui::GetIO();
        io.Fonts->Clear();
        io.Fonts->AddFontDefault(!s_entries.empty() || density > 1.f ? &cfg : nullptr);

        for (auto& entry : s_entries) {
            if (entry.data.empty()) {
                entry.font = nullptr;
                continue;
            }

            ImFontConfig entryCfg = cfg;
            entryCfg.FontDataOwnedByAtlas = false;
            entry.font = io.Fonts->AddFontFromMemoryTTF(
                entry.data.data(), static_cast<int>(entry.data.size()), entry.size, &entryCfg
            );
        }
    }

    void imguiFontAfterRegistryChange() {
        if (imguiHostIsInitialized()) {
            imguiHostRequestReload();
            return;
        }
        imguiFontRebuildAtlas();
    }

    void imguiFontClear() {
        s_entries.clear();
        s_nextId = 1;
    }
} // namespace luax
