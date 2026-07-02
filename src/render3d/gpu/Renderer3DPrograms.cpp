#include "render3d/gpu/Renderer3DPrograms.hpp"

#include "render3d/gpu/GlUtil.hpp"
#include "render3d/gpu/ShaderSources.hpp"

namespace luax::render3d {

    void Renderer3DPrograms::destroyGlPrograms() {
        if (gpuFeaturesDisabled() || !gameTexturesLoaded() || !glContextAvailable()) {
            return;
        }
        if (lambertProgram != 0) {
            glDeleteProgram(lambertProgram);
        }
        if (lambertInstProgram != 0) {
            glDeleteProgram(lambertInstProgram);
        }
        if (instanceVbo != 0) {
            glDeleteBuffers(1, &instanceVbo);
        }
        if (debugLineVbo != 0) {
            glDeleteBuffers(1, &debugLineVbo);
        }
        if (debugLineProgram != 0) {
            glDeleteProgram(debugLineProgram);
        }
    }

    void Renderer3DPrograms::reset() {
        lambertProgram = 0;
        lambertLocMvp = -1;
        lambertLocNormalMat = -1;
        lambertLocLightDir = -1;
        lambertLocLightColor = -1;
        lambertLocAmbient = -1;
        lambertLocBaseColor = -1;
        lambertLocTexture = -1;
        lambertLocUseTexture = -1;
        lambertLocAlphaCutoff = -1;
        lambertLocTint = -1;

        lambertInstProgram = 0;
        lambertInstLocViewProj = -1;
        lambertInstLocLightDir = -1;
        lambertInstLocLightColor = -1;
        lambertInstLocAmbient = -1;
        lambertInstLocBaseColor = -1;
        lambertInstLocTexture = -1;
        lambertInstLocUseTexture = -1;
        lambertInstLocAlphaCutoff = -1;

        instanceVbo = 0;

        debugLineProgram = 0;
        debugLineLocMvp = -1;
        debugLineLocColor = -1;
        debugLineVbo = 0;
    }

    bool Renderer3DPrograms::ensureLambertProgram() {
        if (lambertProgram != 0) {
            return true;
        }

        lambertProgram = buildProgram(
            shader_sources::kLambertVert,
            shader_sources::kLambertFrag,
            "lambert",
            {{0, "aPos"}, {1, "aNormal"}, {2, "aTexCoord"}}
        );
        if (lambertProgram == 0) {
            return false;
        }

        lambertLocMvp = glGetUniformLocation(lambertProgram, "uMVP");
        lambertLocNormalMat = glGetUniformLocation(lambertProgram, "uNormalMat");
        lambertLocLightDir = glGetUniformLocation(lambertProgram, "uLightDir");
        lambertLocLightColor = glGetUniformLocation(lambertProgram, "uLightColor");
        lambertLocAmbient = glGetUniformLocation(lambertProgram, "uAmbient");
        lambertLocBaseColor = glGetUniformLocation(lambertProgram, "uBaseColor");
        lambertLocTexture = glGetUniformLocation(lambertProgram, "uTexture");
        lambertLocUseTexture = glGetUniformLocation(lambertProgram, "uUseTexture");
        lambertLocAlphaCutoff = glGetUniformLocation(lambertProgram, "uAlphaCutoff");
        lambertLocTint = glGetUniformLocation(lambertProgram, "uTint");
        return true;
    }

    bool Renderer3DPrograms::ensureLambertInstProgram() {
        if (lambertInstProgram != 0) {
            return true;
        }

        lambertInstProgram = buildProgram(
            shader_sources::kLambertInstVert,
            shader_sources::kLambertFrag,
            "lambert-inst",
            {{0, "aPos"},
             {1, "aNormal"},
             {2, "aTexCoord"},
             {3, "aModel0"},
             {4, "aModel1"},
             {5, "aModel2"},
             {6, "aModel3"},
             {7, "aTint"}}
        );
        if (lambertInstProgram == 0) {
            return false;
        }

        lambertInstLocViewProj = glGetUniformLocation(lambertInstProgram, "uViewProj");
        lambertInstLocLightDir = glGetUniformLocation(lambertInstProgram, "uLightDir");
        lambertInstLocLightColor = glGetUniformLocation(lambertInstProgram, "uLightColor");
        lambertInstLocAmbient = glGetUniformLocation(lambertInstProgram, "uAmbient");
        lambertInstLocBaseColor = glGetUniformLocation(lambertInstProgram, "uBaseColor");
        lambertInstLocTexture = glGetUniformLocation(lambertInstProgram, "uTexture");
        lambertInstLocUseTexture = glGetUniformLocation(lambertInstProgram, "uUseTexture");
        lambertInstLocAlphaCutoff = glGetUniformLocation(lambertInstProgram, "uAlphaCutoff");
        return true;
    }

    bool Renderer3DPrograms::ensureInstanceVbo() {
        if (instanceVbo != 0) {
            return true;
        }
        glGenBuffers(1, &instanceVbo);
        return instanceVbo != 0;
    }

    bool Renderer3DPrograms::ensureDebugLineProgram() {
        if (debugLineProgram != 0) {
            return true;
        }

        debugLineProgram = buildProgram(
            shader_sources::kDebugLineVert, shader_sources::kDebugLineFrag, "debug-line", {{0, "aPos"}}
        );
        if (debugLineProgram == 0) {
            return false;
        }

        debugLineLocMvp = glGetUniformLocation(debugLineProgram, "uMVP");
        debugLineLocColor = glGetUniformLocation(debugLineProgram, "uColor");
        return true;
    }

    bool Renderer3DPrograms::ensureDebugLineVbo() {
        if (debugLineVbo != 0) {
            return true;
        }
        glGenBuffers(1, &debugLineVbo);
        return debugLineVbo != 0;
    }

} // namespace luax::render3d
