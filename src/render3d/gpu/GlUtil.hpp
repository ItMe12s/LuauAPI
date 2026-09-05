#pragma once

#include <array>
#include <cocos2d.h>

namespace luax::render3d {

    bool glContextAvailable();

    void drainGlErrors();

    unsigned glContextGeneration();
    void bumpGlContextGeneration();

    bool gameTexturesLoaded();
    void markGameTexturesLoaded();
    void markGameTexturesUnloaded();

    bool gpuFeaturesDisabled();
    void disableGpuFeaturesForSession();

    inline bool gpuSessionReady() {
        return !gpuFeaturesDisabled() && gameTexturesLoaded();
    }

    // Used by the gd-imgui-cocos backend render path.
    // Not used by the gd3d draw paths anymore.
    unsigned int genVao();
    void bindVao(unsigned int vao);
    void deleteVao(unsigned int vao);

    struct DrawStateSnapshot {
        GLboolean depthEnabled = GL_FALSE;
        GLboolean depthMask = GL_TRUE;
        GLboolean cullEnabled = GL_FALSE;
        GLboolean blendEnabled = GL_FALSE;
        GLboolean scissorEnabled = GL_FALSE;
        GLint blendSrc = GL_ONE;
        GLint blendDst = GL_ZERO;
        GLint program = 0;
        GLint activeTexture = GL_TEXTURE0;
        GLint unpackAlignment = 4;
        int boundTexture = 0;
        int arrayBuffer = 0;
        int elementArrayBuffer = 0;
        int framebufferBinding = 0;
        std::array<int, 4> viewport{0, 0, 0, 0};
        std::array<int, 4> scissorBox{0, 0, 0, 0};
        std::array<float, 4> clearColor{0.0f, 0.0f, 0.0f, 0.0f};

        void capture();
        void restore() const;
    };

    unsigned int compileShader(unsigned int type, char const* source);

    unsigned int buildProgram(
        char const* vertSource, char const* fragSource, char const* label,
        std::initializer_list<std::pair<unsigned int, char const*>> attribs
    );

} // namespace luax::render3d
