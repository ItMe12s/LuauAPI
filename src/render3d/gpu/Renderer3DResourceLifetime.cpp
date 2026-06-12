#include "render3d/gpu/Renderer3DResourceLifetime.hpp"

#include "render3d/gpu/GlUtil.hpp"
#include "render3d/gpu/Renderer3DMeshCache.hpp"
#include "render3d/gpu/Renderer3DPrograms.hpp"

namespace luax::render3d {

    void destroyRenderer3DGlResources(Renderer3DPrograms& programs, Renderer3DMeshCache& meshCache) {
        meshCache.destroyAllGpuResources();
        programs.destroyGlPrograms();
        meshCache.clear();
        programs.reset();
    }

} // namespace luax::render3d
