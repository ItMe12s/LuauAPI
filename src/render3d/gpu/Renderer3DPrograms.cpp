#include "render3d/gpu/Renderer3DPrograms.hpp"

#include "render3d/gpu/GlUtil.hpp"
#include "render3d/gpu/ShaderSources.hpp"

#include <Geode/Geode.hpp>

namespace luax::render3d {

    GlProgram::~GlProgram() {
        reset();
    }

    void GlProgram::reset() {
        if (id != 0 && glContextAvailable() && gen == glContextGeneration()) {
            glDeleteProgram(id);
        }
        id = 0;
        gen = 0;
    }

    GlBuffer::~GlBuffer() {
        reset();
    }

    void GlBuffer::reset() {
        if (id != 0 && glContextAvailable() && gen == glContextGeneration()) {
            glDeleteBuffers(1, &id);
        }
        id = 0;
        gen = 0;
    }

    void Renderer3DPrograms::destroyGlPrograms() {
        lambert = GlProgram{};
        lambertLocs = LambertLocs{};
        debugLine = GlProgram{};
        debugLineLocs = DebugLineLocs{};
        debugLineVbo = GlBuffer{};
    }

    bool Renderer3DPrograms::ensureLambertProgram() {
        if (lambert.id != 0) {
            return true;
        }

        lambert.id = buildProgram(
            shader_sources::kLambertVert,
            shader_sources::kLambertFrag,
            "lambert",
            {{0, "aPos"}, {1, "aNormal"}, {2, "aTexCoord"}}
        );
        if (lambert.id == 0) {
            return false;
        }
        lambert.gen = glContextGeneration();

        lambertLocs.mvp = glGetUniformLocation(lambert.id, "uMVP");
        lambertLocs.normalMat = glGetUniformLocation(lambert.id, "uNormalMat");
        lambertLocs.lightDir = glGetUniformLocation(lambert.id, "uLightDir");
        lambertLocs.lightColor = glGetUniformLocation(lambert.id, "uLightColor");
        lambertLocs.ambient = glGetUniformLocation(lambert.id, "uAmbient");
        lambertLocs.baseColor = glGetUniformLocation(lambert.id, "uBaseColor");
        lambertLocs.texture = glGetUniformLocation(lambert.id, "uTexture");
        lambertLocs.useTexture = glGetUniformLocation(lambert.id, "uUseTexture");
        lambertLocs.alphaCutoff = glGetUniformLocation(lambert.id, "uAlphaCutoff");
        lambertLocs.tint = glGetUniformLocation(lambert.id, "uTint");
        return true;
    }

    bool Renderer3DPrograms::ensureDebugLineProgram() {
        if (debugLine.id != 0) {
            return true;
        }

        debugLine.id = buildProgram(
            shader_sources::kDebugLineVert, shader_sources::kDebugLineFrag, "debug-line", {{0, "aPos"}}
        );
        if (debugLine.id == 0) {
            return false;
        }
        debugLine.gen = glContextGeneration();

        debugLineLocs.mvp = glGetUniformLocation(debugLine.id, "uMVP");
        debugLineLocs.color = glGetUniformLocation(debugLine.id, "uColor");
        return true;
    }

    bool Renderer3DPrograms::ensureDebugLineVbo() {
        if (debugLineVbo.id != 0) {
            return true;
        }
        drainGlErrors();
        glGenBuffers(1, &debugLineVbo.id);
        if (debugLineVbo.id != 0 && glGetError() == GL_NO_ERROR) {
            debugLineVbo.gen = glContextGeneration();
        }
        else {
            debugLineVbo.id = 0;
        }
        return debugLineVbo.id != 0;
    }

} // namespace luax::render3d
