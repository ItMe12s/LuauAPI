#include "render3d/GlUtil.hpp"

#include <Geode/Geode.hpp>

namespace luax::render3d {

    bool glContextAvailable() {
        auto* director = cocos2d::CCDirector::sharedDirector();
        return director != nullptr && director->getOpenGLView() != nullptr;
    }

    bool vaoSupported() {
#if !defined(GL_VERTEX_ARRAY_BINDING)
        return false;
#elif defined(GLEW_VERSION)
        return glGenVertexArrays != nullptr && glBindVertexArray != nullptr;
#else
        return true;
#endif
    }

    bool instancingSupported() {
#if defined(GLEW_VERSION)
        return glDrawElementsInstanced != nullptr && glVertexAttribDivisor != nullptr;
#else
        return false;
#endif
    }

    int captureAndUnbindVao() {
#if defined(GL_VERTEX_ARRAY_BINDING)
        GLint prevVao = 0;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVao);
        if (prevVao != 0) {
            glBindVertexArray(0);
        }
        return prevVao;
#else
        return 0;
#endif
    }

    void restoreVao([[maybe_unused]] int prevVao) {
#if defined(GL_VERTEX_ARRAY_BINDING)
        if (prevVao != 0) {
            glBindVertexArray(static_cast<GLuint>(prevVao));
        }
#endif
    }

    void DrawStateSnapshot::capture() {
        glGetBooleanv(GL_DEPTH_TEST, &depthEnabled);
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
        glGetBooleanv(GL_CULL_FACE, &cullEnabled);
        glGetBooleanv(GL_BLEND, &blendEnabled);
        glGetIntegerv(GL_BLEND_SRC_RGB, &blendSrc);
        glGetIntegerv(GL_BLEND_DST_RGB, &blendDst);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTexture);
    }

    void DrawStateSnapshot::restore() const {
        if (depthEnabled == GL_TRUE) {
            glEnable(GL_DEPTH_TEST);
        }
        else {
            glDisable(GL_DEPTH_TEST);
        }
        glDepthMask(depthMask);
        if (cullEnabled == GL_TRUE) {
            glEnable(GL_CULL_FACE);
        }
        else {
            glDisable(GL_CULL_FACE);
        }
        if (blendEnabled == GL_TRUE) {
            glEnable(GL_BLEND);
            glBlendFunc(static_cast<GLenum>(blendSrc), static_cast<GLenum>(blendDst));
        }
        else {
            glDisable(GL_BLEND);
        }
        cocos2d::ccGLBindTexture2D(static_cast<GLuint>(boundTexture));
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(boundTexture));
    }

    unsigned int compileShader(unsigned int type, char const* source) {
        unsigned int const shader = glCreateShader(type);
        char const* src = source;
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        int ok = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
        if (ok == GL_TRUE) {
            return shader;
        }

        char log[512]{};
        glGetShaderInfoLog(shader, static_cast<GLsizei>(sizeof log), nullptr, log);
        geode::log::error("Renderer3D: shader compile failed: {}", log);
        glDeleteShader(shader);
        return 0;
    }

    unsigned int buildProgram(
        char const* vertSource, char const* fragSource, char const* label,
        std::initializer_list<std::pair<unsigned int, char const*>> attribs
    ) {
        unsigned int const vert = compileShader(GL_VERTEX_SHADER, vertSource);
        if (vert == 0) {
            return 0;
        }

        unsigned int const frag = compileShader(GL_FRAGMENT_SHADER, fragSource);
        if (frag == 0) {
            glDeleteShader(vert);
            return 0;
        }

        unsigned int program = glCreateProgram();
        for (auto const& [location, name] : attribs) {
            glBindAttribLocation(program, static_cast<GLuint>(location), name);
        }

        glAttachShader(program, vert);
        glAttachShader(program, frag);
        glLinkProgram(program);
        glDeleteShader(vert);
        glDeleteShader(frag);

        int ok = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &ok);
        if (ok != GL_TRUE) {
            char log[512]{};
            glGetProgramInfoLog(program, static_cast<GLsizei>(sizeof log), nullptr, log);
            geode::log::error("Renderer3D: {} program link failed: {}", label, log);
            glDeleteProgram(program);
            return 0;
        }

        return program;
    }

} // namespace luax::render3d
