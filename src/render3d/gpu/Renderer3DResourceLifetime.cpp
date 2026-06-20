#include "framework/lifecycle/Lifecycle.hpp"
#include "render3d/gpu/GlUtil.hpp"
#include "render3d/gpu/Renderer3D.hpp"
#include "render3d/gpu/Renderer3DMeshCache.hpp"
#include "render3d/gpu/Renderer3DPrograms.hpp"

namespace luax::render3d {
    namespace {
        bool& renderer3DShutdownHookRegistered() {
            static bool registered = false;
            return registered;
        }
    } // namespace

    void destroyRenderer3DGlResources(Renderer3DPrograms& programs, Renderer3DMeshCache& meshCache) {
        meshCache.destroyAllGpuResources();
        programs.destroyGlPrograms();
        meshCache.clear();
        programs.reset();
    }

    void clearRenderer3DGlState() {
        Renderer3D::instance().destroyGlResources();
        renderer3DShutdownHookRegistered() = false;
    }

    void ensureRenderer3DShutdownHook() {
        luax::ensureShutdownHook(renderer3DShutdownHookRegistered(), &clearRenderer3DGlState);
    }

} // namespace luax::render3d
