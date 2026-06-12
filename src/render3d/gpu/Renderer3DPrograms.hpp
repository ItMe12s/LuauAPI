#pragma once

namespace luax::render3d {

    struct Renderer3DPrograms {
        unsigned int lambertProgram = 0;
        int lambertLocMvp = -1;
        int lambertLocNormalMat = -1;
        int lambertLocLightDir = -1;
        int lambertLocLightColor = -1;
        int lambertLocAmbient = -1;
        int lambertLocBaseColor = -1;
        int lambertLocTexture = -1;
        int lambertLocUseTexture = -1;
        int lambertLocAlphaCutoff = -1;
        int lambertLocTint = -1;

        unsigned int lambertInstProgram = 0;
        int lambertInstLocViewProj = -1;
        int lambertInstLocLightDir = -1;
        int lambertInstLocLightColor = -1;
        int lambertInstLocAmbient = -1;
        int lambertInstLocBaseColor = -1;
        int lambertInstLocTexture = -1;
        int lambertInstLocUseTexture = -1;
        int lambertInstLocAlphaCutoff = -1;

        unsigned int instanceVbo = 0;

        unsigned int blitProgram = 0;
        int blitLocMvp = -1;
        int blitLocTexture = -1;
        unsigned int blitVbo = 0;

        unsigned int debugLineProgram = 0;
        int debugLineLocMvp = -1;
        int debugLineLocColor = -1;
        unsigned int debugLineVbo = 0;

        bool ensureLambertProgram();
        bool ensureLambertInstProgram();
        bool ensureBlitProgram();
        bool ensureBlitGeometry();
        bool ensureInstanceVbo();
        bool ensureDebugLineProgram();
        bool ensureDebugLineVbo();

        void destroyGlPrograms();
        void reset();
    };

} // namespace luax::render3d
