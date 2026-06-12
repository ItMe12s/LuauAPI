#pragma once

namespace luax::render3d {

    struct Renderer3DPrograms;
    class Renderer3DMeshCache;

    void destroyRenderer3DGlResources(Renderer3DPrograms& programs, Renderer3DMeshCache& meshCache);

} // namespace luax::render3d
