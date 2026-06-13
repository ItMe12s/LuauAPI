#include "render3d/gpu/ViewportComposite.hpp"

#include "render3d/gpu/GlUtil.hpp"

#include <Geode/Geode.hpp>

namespace luax::render3d {
    using cocos2d::ccc4;
    using cocos2d::ccColor4B;
    using cocos2d::ccGLBindTexture2D;
    using cocos2d::ccGLBlendFunc;
    using cocos2d::ccGLEnableVertexAttribs;
    using cocos2d::ccGLUseProgram;
    using cocos2d::CCShaderCache;
    using cocos2d::ccTex2F;
    using cocos2d::ccVertex2F;
    using cocos2d::tex2;
    using cocos2d::vertex2;

    void drawViewportComposite(unsigned int colorTexture, float width, float height) {
        if (colorTexture == 0 || width <= 0.0f || height <= 0.0f) {
            return;
        }

        DrawStateSnapshot prevState{};
        prevState.capture();
        int const prevVao = captureAndUnbindVao();

        ccGLBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        ccGLBindTexture2D(colorTexture);

        auto* shader =
            CCShaderCache::sharedShaderCache()->programForKey(kCCShader_PositionTextureColor);
        if (shader == nullptr) {
            restoreVao(prevVao);
            prevState.restore();
            return;
        }

        shader->use();
        shader->setUniformsForBuiltins();
        ccGLEnableVertexAttribs(cocos2d::kCCVertexAttribFlag_PosColorTex);

        ccVertex2F vertices[] = {
            vertex2(0.0f, 0.0f),
            vertex2(width, 0.0f),
            vertex2(0.0f, height),
            vertex2(width, height),
        };
        ccColor4B const white = ccc4(255, 255, 255, 255);
        ccColor4B colors[] = {white, white, white, white};
        ccTex2F texCoords[] = {
            tex2(0.0f, 0.0f),
            tex2(1.0f, 0.0f),
            tex2(0.0f, 1.0f),
            tex2(1.0f, 1.0f),
        };

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glVertexAttribPointer(cocos2d::kCCVertexAttrib_Position, 2, GL_FLOAT, GL_FALSE, 0, vertices);
        glVertexAttribPointer(cocos2d::kCCVertexAttrib_Color, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, colors);
        glVertexAttribPointer(cocos2d::kCCVertexAttrib_TexCoords, 2, GL_FLOAT, GL_FALSE, 0, texCoords);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glDisableVertexAttribArray(cocos2d::kCCVertexAttrib_Position);
        glDisableVertexAttribArray(cocos2d::kCCVertexAttrib_Color);
        glDisableVertexAttribArray(cocos2d::kCCVertexAttrib_TexCoords);

        restoreVao(prevVao);
        prevState.restore();
        ccGLUseProgram(0);
        ccGLEnableVertexAttribs(cocos2d::kCCVertexAttribFlag_None);
    }

} // namespace luax::render3d
