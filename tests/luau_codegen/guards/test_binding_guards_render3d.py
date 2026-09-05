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
        self.assertIn("m_gpuMeshes.erase(&meshAsset)", body)
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

    def test_viewport_frame_destructor_releases_texture_gpu(self) -> None:
        source = read_repo_file(CC_VIEWPORT_FRAME)
        dtor_match = re.search(
            r"CCViewportFrame::~CCViewportFrame\(\)\s*\{([^}]+)\}",
            source,
            re.DOTALL,
        )
        assert dtor_match is not None, "CCViewportFrame destructor must exist"
        dtor_body = dtor_match.group(1)
        self.assertIn(
            "releaseTextureGpu",
            dtor_body,
            "viewport destruction must release GPU texture",
        )
        self.assertIn("m_viewportTexture.reset()", dtor_body)
        self.assertIn(
            "detachSpriteTexture()",
            dtor_body,
            "viewport destruction must detach the sprite texture before the "
            "framebuffer deletes the color name",
        )

    def test_strict_gles2_contract_has_no_instancing_and_uses_u16(self) -> None:
        util = read_repo_file(RENDERER3D_GL_UTIL)
        self.assertNotIn("glVertexAttribDivisor", util)
        self.assertNotIn("glDrawElementsInstanced", util)

        scene_pass = read_repo_file("src/render3d/gpu/Renderer3DScenePass.cpp")
        self.assertNotIn("glDrawElementsInstanced", scene_pass)
        self.assertNotIn("glVertexAttribDivisor", scene_pass)
        self.assertIn("GL_UNSIGNED_SHORT", scene_pass)

        mesh_cache = read_repo_file(RENDERER3D_MESH_CACHE)
        self.assertNotIn("glVertexAttribDivisor", mesh_cache)
        self.assertIn("std::uint16_t", mesh_cache)
