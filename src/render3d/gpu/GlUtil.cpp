#include "render3d/gpu/GlUtil.hpp"

#include <Geode/Geode.hpp>

namespace {
    static unsigned s_glContextGeneration = 0;
    static bool s_gameTexturesLoaded = false;
} // namespace

namespace luax::render3d {

    bool gameTexturesLoaded() {
        return s_gameTexturesLoaded;
    }

    void markGameTexturesLoaded() {
        s_gameTexturesLoaded = true;
    }

    void markGameTexturesUnloaded() {
        s_gameTexturesLoaded = false;
    }

    unsigned glContextGeneration() {
        return s_glContextGeneration;
    }

    void bumpGlContextGeneration() {
        ++s_glContextGeneration;
    }

    bool glContextAvailable() {
        auto* director = cocos2d::CCDirector::sharedDirector();
        return director != nullptr && director->getOpenGLView() != nullptr;
    }

    void drainGlErrors() {
        while (glGetError() != GL_NO_ERROR) {}
    }

    namespace {
        bool vaoExtensionPresent() {
#if defined(GL_VERTEX_ARRAY_BINDING)
            return true;
#else
            return false;
#endif
        }
    } // namespace

    unsigned int genVao() {
        if (!vaoExtensionPresent()) {
            return 0;
        }
#if defined(GL_VERTEX_ARRAY_BINDING)
        GLuint vao = 0;
        glGenVertexArrays(1, &vao);
        return vao;
#else
        return 0;
#endif
    }

    void bindVao(unsigned int vao) {
#if defined(GL_VERTEX_ARRAY_BINDING)
        if (vaoExtensionPresent()) {
            glBindVertexArray(vao);
        }
#else
        (void)vao;
#endif
    }

    void deleteVao(unsigned int vao) {
#if defined(GL_VERTEX_ARRAY_BINDING)
        if (vao != 0 && glContextAvailable() && vaoExtensionPresent()) {
            GLuint doomed = vao;
            glDeleteVertexArrays(1, &doomed);
        }
#else
        (void)vao;
#endif
    }

    void DrawStateSnapshot::capture() {
        glGetBooleanv(GL_DEPTH_TEST, &depthEnabled);
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
        glGetBooleanv(GL_CULL_FACE, &cullEnabled);
        glGetBooleanv(GL_BLEND, &blendEnabled);
        glGetBooleanv(GL_SCISSOR_TEST, &scissorEnabled);
        glGetIntegerv(GL_BLEND_SRC_RGB, &blendSrc);
        glGetIntegerv(GL_BLEND_DST_RGB, &blendDst);
        glGetIntegerv(GL_CURRENT_PROGRAM, &program);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTexture);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &arrayBuffer);
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &elementArrayBuffer);
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &framebufferBinding);
        glGetIntegerv(GL_VIEWPORT, viewport.data());
        glGetIntegerv(GL_SCISSOR_BOX, scissorBox.data());
        glGetFloatv(GL_COLOR_CLEAR_VALUE, clearColor.data());
        glActiveTexture(static_cast<GLenum>(activeTexture));
    }

    void DrawStateSnapshot::restore() const {
        glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(framebufferBinding));
        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        if (scissorEnabled == GL_TRUE) {
            glEnable(GL_SCISSOR_TEST);
            glScissor(scissorBox[0], scissorBox[1], scissorBox[2], scissorBox[3]);
        }
        else {
            glDisable(GL_SCISSOR_TEST);
        }
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
        glBlendFunc(static_cast<GLenum>(blendSrc), static_cast<GLenum>(blendDst));
        if (blendEnabled == GL_TRUE) {
            glEnable(GL_BLEND);
        }
        else {
            glDisable(GL_BLEND);
        }
        glUseProgram(static_cast<GLuint>(program));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(boundTexture));
        cocos2d::ccGLEnableVertexAttribs(cocos2d::kCCVertexAttribFlag_None);
        glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(arrayBuffer));
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLuint>(elementArrayBuffer));
        glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
        glActiveTexture(static_cast<GLenum>(activeTexture));
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

        char log[2048]{};
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
            char log[2048]{};
            glGetProgramInfoLog(program, static_cast<GLsizei>(sizeof log), nullptr, log);
            geode::log::error("Renderer3D: {} program link failed: {}", label, log);
            glDeleteProgram(program);
            return 0;
        }

        return program;
    }

} // namespace luax::render3d
