# gd3d

## Summary

`gd3d` provides lightweight 3D rendering inside the cocos2d scene graph.
Load glTF meshes, place them in a `ViewportFrame` sprite, and attach that node like any other scene node.
For signatures, use editor autocomplete from [type stubs](type-stubs.md).

The namespace has six parts:

- `gd3d.Transform` for position and rotation
- `gd3d.gltf` for loading mesh assets
- `gd3d.mesh` for building meshes from vertex data
- `gd3d.texture` for loading standalone image textures
- `gd3d.Material` for solid-color, textured, or glTF-derived materials
- `gd3d.ViewportFrame` for the render target sprite

Vec3 values are plain tables `{ x, y, z }`. `ViewportFrame` is a `CCSprite` subclass.
It still exposes `CCNode` layout methods (`addChild`, `setPosition`, `setContentSize`, and so on).

## Transform

Construct with `Transform.new()`, `Transform.new(position)`, `Transform.new(position, lookAt)`,
`Transform.fromEuler(pitch, yaw, roll)`, or `Transform.fromAxisAngle(axis, angleRadians)`.
Euler and axis-angle angles are in radians.
`Transform.new(position, lookAt)` builds a look-at rotation with +Y as up.

Transforms are immutable. `withPosition` and `withRotation` return new values.
Multiply two transforms with `transform * otherTransform`.
Read back position, basis vectors, and euler angles through instance methods.
`eulerAngles` returns pitch, yaw, and roll in radians.

## glTF loading

`gd3d.gltf.loadMesh(root, path)` loads a glTF 2.0 file (`.glb` or `.gltf`) from a mod sandbox root.
`gd3d.gltf.loadMeshFromBytes(bytes)` loads glTF data already in memory. Pass a Luau `buffer` or raw `string`.
Use GLB with embedded buffers and images only.
External URI references fail with the loader's usual buffer or image errors.
See [Limits and errors](../cpp/limits-and-errors.md) for the read cap.
The first argument uses the same roots as [fs](fs.md): `"save"`, `"config"`, `"persistent"`, or `"resources"`.
Shipped assets belong under `"resources"`.

Returns a mesh handle, or `nil` and an error message. See [globals](globals.md) Error shapes.
`addMesh` keeps the mesh alive. You do not need to keep the handle after adding.

Supported content:

- Embedded and external buffers inside the sandbox root
- Triangle primitives only
- Node world transforms baked into vertex positions and normals
- glTF materials: `pbr_metallic_roughness.base_color_factor`
- glTF material flags: `alphaMode` (`OPAQUE`, `MASK`, `BLEND`), `alphaCutoff`, `doubleSided`
- Base color textures (PNG or JPEG), embedded in GLB or referenced inside the sandbox root
- `TEXCOORD_0` on primitives that use a base color texture

Not supported:

- Draco compression
- meshopt compression
- Sparse accessors
- Metallic/roughness maps, normal maps, emissive textures
- KHR texture extensions (BasisU, WebP, and similar)
- Multisample anti-aliasing (MSAA)

Primitives with a textured material must include `TEXCOORD_0`, loading fails otherwise.

All mesh nodes merge into one asset. Node transforms bake into vertices and normals.
No scene hierarchy, animation, morph targets, or glTF cameras and lights.
Reloading creates a new handle and uploads GPU data again.

`vertexCount` is the total vertex count across all primitives.
`primitiveCount` is the number of triangle groups in the file.
`boundingBox` returns world-space bounds after node transforms are baked.
When the mesh has no geometry, `empty` is `true` and `min` and `max` are zero.

`getMaterial` uses 0-based indices, matching glTF material indices.
Valid range is `0` to `materialCount() - 1`. It returns `nil` when the index is out of range.
Materials from a mesh keep the mesh data alive and may include a base color texture from the glTF file.

## Procedural meshes

`gd3d.mesh.new(data)` builds a mesh from CPU-side geometry without a glTF file.
Returns the same `Mesh` handle type as `gd3d.gltf.loadMesh`,
so it works with `viewport:addMesh`, materials, and the rest of the gd3d pipeline.

`positions` is required and must contain at least one vertex.
See [Limits and errors](../cpp/limits-and-errors.md) for the vertex cap.
`indices` is required. Use 1-based vertex indices (Luau convention).
Each group of three entries is one triangle. The count must be a multiple of three.

If `normals` is omitted, flat normals are computed from face normals.
`normals` and `uvs`, when provided, must match `positions` length.

The mesh has one primitive and no embedded materials (`materialCount()` is `0`).
Set a material with `viewport:addMesh` or `viewport:setInstanceMaterial`.

## Texture loading

`gd3d.texture.load(root, path)` loads a PNG or JPEG image from a mod sandbox root.
The first argument uses the same roots as [fs](fs.md).
Returns a texture handle, or `nil` and an error message. See [globals](globals.md) Error shapes.
Texture data is released when the handle is collected and no material references it anymore.
Materials that reference a texture keep it alive on their own.
Viewport render targets from `viewport:texture()` are also `Texture` handles with the same methods.

## Material

`gd3d.Material.new({ color, texture? })` creates a material with a solid base color.
Pass an optional `texture` field to sample an image loaded with `gd3d.texture.load`.
Use `{ r, g, b, a }` for RGBA, or a `Vec3` `{ x, y, z }` for opaque RGB.
`hasTexture` is `true` for a loaded image, viewport render target, or glTF base color texture.

### Instance override

By default each primitive uses its glTF `materialIndex`.
Pass a fourth argument to `addMesh`, or call `setInstanceMaterial`, to replace all primitives with one material.
Pass `nil` to clear that override.

`setInstancePrimitiveMaterial` overrides one primitive (0-based index).
Per-primitive overrides beat the instance-wide override.
`setInstanceColor` tints the shaded result. Default is white `{ x = 1, y = 1, z = 1 }`.

## ViewportFrame

`gd3d.ViewportFrame.new(width, height)` renders its 3D scene into an off-screen buffer, then draws that texture over its content rect.
Set the node size with `setContentSize` or pass width and height to `new`.
The node uses a centered anchor point (`0.5, 0.5`), so `setPosition` places the middle of the viewport at the given point.

`addMesh` returns an instance id. The optional `material` sets an instance-wide override.
Setter methods return `false` for an unknown id. Readback methods return `nil`.

The camera transform is the camera world pose. The renderer uses its inverse for the view matrix.
Configure the camera with `setCamera(transform, fovDeg, near, far)`.

Background color clears the off-screen buffer. Default is transparent `{ r = 0, g = 0, b = 0, a = 0 }`.
Alpha below 1 lets the 2D scene show through.

`setLight` sets direction, color, and intensity. Zero-length direction errors.
`setAmbient` clamps to `[0, 4]`. Defaults: direction `{ x = 0.35, y = 0.85, z = 0.4 }`, white light, intensity `1`, ambient `0.15`.

`setCompositeEnabled(false)` skips compositing into the 2D scene. The 3D pass still runs so `texture()` works.

`texture()` returns a `Texture` for the color buffer. Use it in `gd3d.Material.new{ texture = ... }`.
Content is from the previous render in the same frame. Resizing recreates the framebuffer.

`addDebugLine` draws a world-space line and returns an id. `setDebugBounds(true)` draws green boxes around instance bounds.
Debug lines read depth but do not write it. Full mesh wireframe is not supported.

### Rendering model

Drawing needs an active OpenGL context. If it is not ready, framebuffer setup and draws are skipped.

The viewport renders to an off-screen framebuffer at the cocos content scale.
When compositing is enabled, the sprite draws that framebuffer texture into the scene.
Opaque and mask draws run first. Blend draws run second, sorted back to front.
`doubleSided` disables culling. Off-screen instances are frustum culled.
Opaque draws are sorted by texture and mesh. VAOs and GPU instancing are used when supported.

Reloading a mesh uploads fresh GPU data.

See [Examples](../../getting-started/examples.md) and [src/scripts/_viewportdemo.luau](../../../src/scripts/_viewportdemo.luau).

## Limits

See [Limits and errors](../cpp/limits-and-errors.md) for mesh, texture, procedural caps, and GPU session disable.

## Finding signatures

The authoritative argument lists live in the generated type stubs, surfaced as editor autocomplete.
See [type stubs](type-stubs.md).

## Related

- [fs](fs.md)
- [type stubs](type-stubs.md)
- [Examples](../../getting-started/examples.md)
- [Getting started](../../getting-started/overview.md)
- [globals](globals.md)
- [game objects](game-objects.md)
- [UI and layouts](ui.md)
- [LuauAPI mod guidelines](../../mod_guidelines.md)
- [Limits and errors](../cpp/limits-and-errors.md)

## Source

- `src/bindings/render3d/internal/Marshaling.hpp`
- `src/bindings/render3d/internal/Handles.hpp`
- `src/bindings/render3d/internal/MeshHandleBinding.cpp`
- `src/bindings/render3d/Gd3dRegister.cpp`
- `src/bindings/render3d/TransformBinding.cpp`
- `src/bindings/render3d/GltfBinding.cpp`
- `src/bindings/render3d/ProceduralMeshBinding.cpp`
- `src/bindings/render3d/TextureBinding.cpp`
- `src/bindings/render3d/MaterialBinding.cpp`
- `src/bindings/render3d/ViewportFrameBinding.cpp`
- `src/render3d/types/Transform3D.hpp`
- `src/render3d/types/Material.hpp`
- `src/render3d/types/SceneTypes.hpp`
- `src/render3d/assets/MeshAsset.hpp`
- `src/render3d/assets/TextureAsset.hpp`
- `src/render3d/assets/GltfIo.hpp`
- `src/render3d/assets/ImageDecode.hpp`
- `src/render3d/gpu/GlUtil.hpp`
- `src/render3d/gpu/GpuSessionDisable.cpp`
- `src/render3d/viewport/CCViewportFrame.hpp`
- `src/render3d/gpu/Renderer3D.hpp`
