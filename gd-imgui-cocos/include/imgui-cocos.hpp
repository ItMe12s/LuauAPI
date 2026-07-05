#pragma once

#include <MoveOnlyFunction.hpp>
#include <cocos2d.h>
#include <imgui.h>

class ImGuiCocos {
public:

private:
    bool m_initialized = false;
    bool m_visible = true;
    bool m_reloading = false;
    bool m_abandonReloadTextures = false;
    luauapi::move_only_function<void()> m_setupCall, m_drawCall;
    ImGuiMouseCursor m_lastCursor = ImGuiMouseCursor_COUNT;
    float m_displayScale = 4.0f;
    void updateTexture(ImTextureData*) const;

    ImGuiCocos();

    void newFrame();
    static ImVec2 displaySize();
    void renderFrame() const;
    void legacyRenderFrame() const; // uses OpenGL 2.0 for rendering, for compatibility with older devices

public:
    ImGuiCocos(ImGuiCocos const&) = delete;
    ImGuiCocos(ImGuiCocos&&) = delete;

    static ImGuiCocos& get();

    // called on mod unloaded
    void destroy(bool abandonTextures = false);
    // called on swapBuffers
    void drawFrame();

    ImGuiCocos& setup(luauapi::move_only_function<void()> fun);
    ImGuiCocos& setup();

    ImGuiCocos& draw(luauapi::move_only_function<void()> fun);

    // used to reinitialize imgui context
    void reload(bool abandonTextures = false);

    void toggle();
    [[nodiscard]] bool isVisible() const;
    void setVisible(bool v);

    // Higher = smaller UI.
    void setDisplayScale(float scale);
    [[nodiscard]] float getDisplayScale() const;

    [[nodiscard]] bool isInitialized() const;

    static ImVec2 cocosToFrame(cocos2d::CCPoint const& pos);
    static cocos2d::CCPoint frameToCocos(ImVec2 const& pos);
};
