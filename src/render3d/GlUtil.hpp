#pragma once

#include <cocos2d.h>
#include <initializer_list>
#include <utility>

namespace luax::render3d {

    bool glContextAvailable();

    bool vaoSupported();

    int captureAndUnbindVao();
    void restoreVao(int prevVao);

    struct DrawStateSnapshot {
        GLboolean depthEnabled = GL_FALSE;
        GLboolean depthMask = GL_TRUE;
        GLboolean cullEnabled = GL_FALSE;
        GLboolean blendEnabled = GL_FALSE;
        GLint blendSrc = GL_ONE;
        GLint blendDst = GL_ZERO;
        int boundTexture = 0;

        void capture();
        void restore() const;
    };

    unsigned int compileShader(unsigned int type, char const* source);

    unsigned int buildProgram(
        char const* vertSource, char const* fragSource, char const* label,
        std::initializer_list<std::pair<unsigned int, char const*>> attribs
    );

} // namespace luax::render3d
