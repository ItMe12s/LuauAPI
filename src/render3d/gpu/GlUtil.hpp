#pragma once

#include <cocos2d.h>
#include <initializer_list>
#include <utility>

namespace luax::render3d {

    bool glContextAvailable();

    unsigned glContextGeneration();
    void bumpGlContextGeneration();

    bool gameTexturesLoaded();
    void markGameTexturesLoaded();
    void markGameTexturesUnloaded();

    bool gpuFeaturesDisabled();
    void disableGpuFeaturesForSession();

    bool vaoSupported();

    bool instancingSupported();

    int captureAndUnbindVao();
    void restoreVao(int prevVao);

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
        int boundTexture = 0;
        int arrayBuffer = 0;
        int elementArrayBuffer = 0;
        int framebufferBinding = 0;
        int vao = 0;
        int viewport[4]{0, 0, 0, 0};
        int scissorBox[4]{0, 0, 0, 0};
        float clearColor[4]{0.0f, 0.0f, 0.0f, 0.0f};

        void capture();
        void restore() const;
    };

    unsigned int compileShader(unsigned int type, char const* source);

    unsigned int buildProgram(
        char const* vertSource, char const* fragSource, char const* label,
        std::initializer_list<std::pair<unsigned int, char const*>> attribs
    );

} // namespace luax::render3d
