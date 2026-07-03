#pragma once

#include <backends/imgui_impl_null.h>
#include <imgui.h>

namespace luauapi_test {
    struct ImGuiTestContext {
        ImGuiTestContext() {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGui_ImplNullPlatform_Init();
            ImGui_ImplNullRender_Init();
        }

        ImGuiTestContext(ImGuiTestContext const&) = delete;
        ImGuiTestContext& operator=(ImGuiTestContext const&) = delete;

        ~ImGuiTestContext() {
            ImGui_ImplNullRender_Shutdown();
            ImGui_ImplNullPlatform_Shutdown();
            ImGui::DestroyContext();
        }
    };

    inline void beginImGuiTestFrame() {
        ImGui_ImplNullPlatform_NewFrame();
        ImGui_ImplNullRender_NewFrame();
        ImGui::NewFrame();
    }

    inline void endImGuiTestFrame() {
        ImGui::Render();
        ImGui_ImplNullRender_RenderDrawData(ImGui::GetDrawData());
    }
} // namespace luauapi_test
