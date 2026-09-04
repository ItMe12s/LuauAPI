#pragma once

namespace luax::render3d {

    struct GlProgram {
        unsigned int id = 0;

        GlProgram() = default;
        GlProgram(GlProgram const&) = delete;
        GlProgram& operator=(GlProgram const&) = delete;

        GlProgram(GlProgram&& other) noexcept : id(other.id) {
            other.id = 0;
        }

        GlProgram& operator=(GlProgram&& other) noexcept {
            if (this != &other) {
                reset();
                id = other.id;
                other.id = 0;
            }
            return *this;
        }

        ~GlProgram();

        void reset();
    };

    struct GlBuffer {
        unsigned int id = 0;

        GlBuffer() = default;
        GlBuffer(GlBuffer const&) = delete;
        GlBuffer& operator=(GlBuffer const&) = delete;

        GlBuffer(GlBuffer&& other) noexcept : id(other.id) {
            other.id = 0;
        }

        GlBuffer& operator=(GlBuffer&& other) noexcept {
            if (this != &other) {
                reset();
                id = other.id;
                other.id = 0;
            }
            return *this;
        }

        ~GlBuffer();

        void reset();
    };

    struct LambertLocs {
        int mvp = -1;
        int normalMat = -1;
        int lightDir = -1;
        int lightColor = -1;
        int ambient = -1;
        int baseColor = -1;
        int texture = -1;
        int useTexture = -1;
        int alphaCutoff = -1;
        int tint = -1;
    };

    struct DebugLineLocs {
        int mvp = -1;
        int color = -1;
    };

    struct Renderer3DPrograms {
        GlProgram lambert{};
        LambertLocs lambertLocs{};
        GlBuffer debugLineVbo{};
        GlProgram debugLine{};
        DebugLineLocs debugLineLocs{};

        bool ensureLambertProgram();
        bool ensureDebugLineProgram();
        bool ensureDebugLineVbo();

        void destroyGlPrograms();
    };

} // namespace luax::render3d
