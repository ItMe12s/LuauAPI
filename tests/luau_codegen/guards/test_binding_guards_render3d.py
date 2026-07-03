from __future__ import annotations

import re
import unittest

from binding_guard_support import (
    CC_VIEWPORT_FRAME,
    RENDERER3D,
    RENDERER3D_GL_UTIL,
    RENDERER3D_LIFETIME,
    RENDERER3D_MESH_CACHE,
    RENDERER3D_TEXTURE2D,
    assert_gl_context_guard,
    function_body,
    read_repo_file,
)


class Render3DGuardTests(unittest.TestCase):
    def test_renderer3d_shutdown_hook_destroys_gl_resources(self) -> None:
        lifetime = read_repo_file(RENDERER3D_LIFETIME)
        self.assertIn("destroyGlResources", lifetime)
        self.assertIn("ensureShutdownHook(renderer3DShutdownHookRegistered()", lifetime)
        clear_body = function_body(lifetime, "clearRenderer3DGlState", ret="void")
        self.assertIn("Renderer3D::instance().destroyGlResources()", clear_body)

        renderer = read_repo_file(RENDERER3D)
        self.assertGreaterEqual(
            renderer.count("ensureRenderer3DShutdownHook()"),
            3,
            "Renderer3D GPU entry points must register the shutdown hook on first use",
        )

    def test_ensure_gpu_mesh_does_not_retain_failed_uploads(self) -> None:
        source = read_repo_file(RENDERER3D_MESH_CACHE)
        body = function_body(source, "Renderer3DMeshCache::ensureGpuMesh", ret="GpuMesh*")
        self.assertIn("hasDrawableGpuPrimitive", body)
        self.assertIn("m_gpuMeshes.erase(meshId)", body)
        self.assertIn("return nullptr", body)

    def test_render3d_gl_paths_guard_missing_context(self) -> None:
        mesh_cache = read_repo_file(RENDERER3D_MESH_CACHE)
        delete_mesh_body = function_body(
            mesh_cache, "Renderer3DMeshCache::deleteGpuMesh", ret="void"
        )
        assert_gl_context_guard(delete_mesh_body, fn="deleteGpuMesh")

        ensure_mesh_body = function_body(
            mesh_cache, "Renderer3DMeshCache::ensureGpuMesh", ret="GpuMesh*"
        )
        self.assertIn("glContextAvailable()", ensure_mesh_body)

        ensure_tex_body = function_body(
            mesh_cache, "Renderer3DMeshCache::ensureGpuTexture", ret="unsigned int"
        )
        self.assertIn("glContextAvailable()", ensure_tex_body)

        texture_body = function_body(
            read_repo_file(RENDERER3D_TEXTURE2D),
            "uploadRgbaTexture2D",
            ret="unsigned int",
        )
        self.assertIn("glContextAvailable()", texture_body)

        delete_vao_body = function_body(read_repo_file(RENDERER3D_GL_UTIL), "deleteVao", ret="void")
        self.assertIn("glContextAvailable()", delete_vao_body)

    def test_viewport_frame_destructor_releases_texture_registry(self) -> None:
        source = read_repo_file(CC_VIEWPORT_FRAME)
        dtor_match = re.search(
            r"CCViewportFrame::~CCViewportFrame\(\)\s*\{([^}]+)\}",
            source,
            re.DOTALL,
        )
        assert dtor_match is not None, "CCViewportFrame destructor must exist"
        dtor_body = dtor_match.group(1)
        self.assertIn(
            "releaseViewportTexture()",
            dtor_body,
            "viewport destruction must release TextureRegistry entries",
        )
        release_body = function_body(source, "CCViewportFrame::releaseViewportTexture", ret="void")
        self.assertIn("TextureRegistry::instance().release", release_body)
        self.assertIn("Renderer3D::instance().releaseTextureGpu", release_body)
        self.assertIn("m_viewportTextureId = 0", release_body)
