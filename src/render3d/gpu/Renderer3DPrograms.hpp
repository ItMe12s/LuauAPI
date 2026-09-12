#pragma once

namespace luax::render3d {

    struct GlProgramTag {};

    struct GlBufferTag {};

    template <class Tag>
    struct GlHandle {
        unsigned int id = 0;
        unsigned int gen = 0;

        GlHandle() = default;
        GlHandle(GlHandle const&) = delete;
        GlHandle& operator=(GlHandle const&) = delete;

        GlHandle(GlHandle&& other) noexcept : id(other.id), gen(other.gen) {
            other.id = 0;
            other.gen = 0;
        }

        GlHandle& operator=(GlHandle&& other) noexcept {
            if (this != &other) {
                reset();
                id = other.id;
                gen = other.gen;
                other.id = 0;
                other.gen = 0;
            }
            return *this;
        }

        ~GlHandle();

        void reset();
    };

    using GlProgram = GlHandle<GlProgramTag>;
    using GlBuffer = GlHandle<GlBufferTag>;

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
