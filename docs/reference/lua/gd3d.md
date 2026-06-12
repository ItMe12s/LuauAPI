# gd3d

## Summary

`gd3d` provides lightweight 3D rendering inside the cocos2d scene graph.
Load glTF meshes, place them in a `ViewportFrame` node, and attach that node like any other `CCNode`.

The namespace has six parts:

- `gd3d.Transform` for position and rotation
- `gd3d.gltf` for loading mesh assets
- `gd3d.mesh` for building meshes from vertex data
- `gd3d.texture` for loading standalone image textures
- `gd3d.Material` for solid-color, textured, or glTF-derived materials
- `gd3d.ViewportFrame` for the render target node

Vec3 values are plain tables `{ x, y, z }`.

## Types

```lua
type Vec3 = { x: number, y: number, z: number }

type Transform = {
    inverse: (self: Transform) -> Transform,
    lerp: (self: Transform, goal: Transform, alpha: number) -> Transform,
    position: (self: Transform) -> Vec3,
    lookVector: (self: Transform) -> Vec3,
    rightVector: (self: Transform) -> Vec3,
    upVector: (self: Transform) -> Vec3,
    withPosition: (self: Transform, pos: Vec3) -> Transform,
    withRotation: (self: Transform, rot: Transform) -> Transform,
    eulerAngles: (self: Transform) -> Vec3,
}

type Material = {
    baseColor: (self: Material) -> { r: number, g: number, b: number, a: number },
    hasTexture: (self: Material) -> boolean,
}

type Texture = {
    width: (self: Texture) -> number,
    height: (self: Texture) -> number,
}

type Mesh = {
    vertexCount: (self: Mesh) -> number,
    primitiveCount: (self: Mesh) -> number,
    boundingBox: (self: Mesh) -> { min: Vec3, max: Vec3, empty: boolean },
    materialCount: (self: Mesh) -> number,
    getMaterial: (self: Mesh, index: number) -> Material?,
}

type ViewportFrame = CCNode & {
    setCamera: (self: ViewportFrame, transform: Transform, fovDeg: number, near: number, far: number) -> (),
    getCamera: (self: ViewportFrame) -> { transform: Transform, fovY: number, near: number, far: number },
    setBackgroundColor: (self: ViewportFrame, color: Vec3 | { r: number, g: number, b: number, a: number? }) -> (),
    getBackgroundColor: (self: ViewportFrame) -> { r: number, g: number, b: number, a: number },
    setLight: (self: ViewportFrame, direction: Vec3, color: Vec3?, intensity: number?) -> (),
    setAmbient: (self: ViewportFrame, ambient: number) -> (),
    getLight: (self: ViewportFrame) -> { direction: Vec3, color: Vec3, intensity: number, ambient: number },
    addMesh: (self: ViewportFrame, mesh: Mesh, transform: Transform, material: Material?) -> number,
    setInstanceMaterial: (self: ViewportFrame, id: number, material: Material?) -> boolean,
    setInstancePrimitiveMaterial: (self: ViewportFrame, id: number, primitiveIndex: number, material: Material?) -> boolean,
    setInstanceColor: (self: ViewportFrame, id: number, color: Vec3) -> boolean,
    setInstanceTransform: (self: ViewportFrame, id: number, transform: Transform) -> boolean,
    getInstanceTransform: (self: ViewportFrame, id: number) -> Transform?,
    getInstanceColor: (self: ViewportFrame, id: number) -> Vec3?,
    getInstanceMaterial: (self: ViewportFrame, id: number) -> Material?,
    getInstancePrimitiveMaterial: (self: ViewportFrame, id: number, primitiveIndex: number) -> Material?,
    getInstanceIds: (self: ViewportFrame) -> { number },
    instanceCount: (self: ViewportFrame) -> number,
    removeInstance: (self: ViewportFrame, id: number) -> boolean,
    clearInstances: (self: ViewportFrame) -> (),
    setCompositeEnabled: (self: ViewportFrame, enabled: boolean) -> (),
    texture: (self: ViewportFrame) -> Texture,
    addDebugLine: (self: ViewportFrame, from: Vec3, to: Vec3, color: Vec3) -> number,
    removeDebugLine: (self: ViewportFrame, id: number) -> boolean,
    clearDebugLines: (self: ViewportFrame) -> (),
    setDebugBounds: (self: ViewportFrame, enabled: boolean) -> (),
}
```

`ViewportFrame` inherits all `CCNode` methods (`addChild`, `setPosition`, `setContentSize`, and so on).

## Transform

```lua
gd3d.Transform.new() -> Transform
gd3d.Transform.new(position: Vec3) -> Transform
gd3d.Transform.new(position: Vec3, lookAt: Vec3) -> Transform
gd3d.Transform.fromEuler(pitch: number, yaw: number, roll: number) -> Transform
gd3d.Transform.fromAxisAngle(axis: Vec3, angleRadians: number) -> Transform
```

Euler and axis-angle angles are in radians.
`Transform.new(position, lookAt)` builds a look-at rotation with +Y as up.

Instance methods:

```lua
transform:inverse() -> Transform
transform:lerp(goal: Transform, alpha: number) -> Transform
transform:position() -> Vec3
transform:lookVector() -> Vec3
transform:rightVector() -> Vec3
transform:upVector() -> Vec3
transform:withPosition(pos: Vec3) -> Transform
transform:withRotation(rot: Transform) -> Transform
transform:eulerAngles() -> Vec3
transform * otherTransform -> Transform
```

Transforms are immutable. `withPosition` and `withRotation` return new values.
`eulerAngles` returns pitch, yaw, and roll in radians.

## glTF loading

```lua
gd3d.gltf.loadMesh(root: FsRoot, path: string) -> (Mesh?, string?)
gd3d.gltf.loadMeshFromBytes(bytes: buffer | string) -> (Mesh?, string?)
```

`loadMesh` loads a glTF 2.0 file (`.glb` or `.gltf`) from a mod sandbox root.
`loadMeshFromBytes` loads glTF data already in memory. Pass a Luau `buffer` or raw `string`.
Use GLB with embedded buffers and images only.
External URI references fail with the loader's usual buffer or image errors.
The same 32 MiB read cap as `loadMesh` applies.
The first argument uses the same roots as [fs](fs.md): `"save"`, `"config"`, `"persistent"`, or `"resources"`.
Shipped assets belong under `"resources"`.

Returns a mesh handle, or `nil` and an error message.
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

Mesh methods:

```lua
mesh:vertexCount() -> number
mesh:primitiveCount() -> number
mesh:boundingBox() -> { min: Vec3, max: Vec3, empty: boolean }
mesh:materialCount() -> number
mesh:getMaterial(index: number) -> Material?
```

`vertexCount` is the total vertex count across all primitives.
`primitiveCount` is the number of triangle groups in the file.
`boundingBox` returns world-space bounds after node transforms are baked.
When the mesh has no geometry, `empty` is `true` and `min` and `max` are zero.

`getMaterial` uses 0-based indices, matching glTF material indices.
Valid range is `0` to `materialCount() - 1`. It returns `nil` when the index is out of range.
Materials from a mesh keep the mesh data alive and may include a base color texture from the glTF file.

## Procedural meshes

```lua
gd3d.mesh.new(data: {
    positions: { Vec3 },
    indices: { number },
    normals: { Vec3 }?,
    uvs: { { x: number, y: number } }?,
}) -> (Mesh?, string?)
```

Builds a mesh from CPU-side geometry without a glTF file.
Returns the same `Mesh` handle type as `gd3d.gltf.loadMesh`,
so it works with `viewport:addMesh`, materials, and the rest of the gd3d pipeline.

`positions` is required and must contain at least one vertex (maximum 200,000).
`indices` is required. Use 1-based vertex indices (Luau convention).
Each group of three entries is one triangle. The count must be a multiple of three.

If `normals` is omitted, flat normals are computed from face normals.
`normals` and `uvs`, when provided, must match `positions` length.

The mesh has one primitive and no embedded materials (`materialCount()` is `0`).
Set a material with `viewport:addMesh` or `viewport:setInstanceMaterial`.

## Texture loading

```lua
gd3d.texture.load(root: FsRoot, path: string) -> (Texture?, string?)
```

Loads a PNG or JPEG image from a mod sandbox root.
The first argument uses the same roots as [fs](fs.md).
Returns a texture handle, or `nil` and an error message.
Texture data is released when the handle is collected and no material references it anymore.
Materials that reference a texture keep it alive on their own.
Viewport render targets from `viewport:texture()` are also `Texture` handles with the same methods.

Texture methods:

```lua
texture:width() -> number
texture:height() -> number
```

## Material

```lua
gd3d.Material.new({
    color: Vec3 | { r: number, g: number, b: number, a: number? },
    texture: Texture?,
}) -> Material
```

Creates a material with a solid base color.
Pass an optional `texture` field to sample an image loaded with `gd3d.texture.load`.
Use `{ r, g, b, a }` for RGBA, or a `Vec3` `{ x, y, z }` for opaque RGB.

Instance methods:

```lua
material:baseColor() -> { r: number, g: number, b: number, a: number }
material:hasTexture() -> boolean
```

`hasTexture` is `true` for a loaded image, viewport render target, or glTF base color texture.

### Instance override

By default each primitive uses its glTF `materialIndex`.
Pass a fourth argument to `addMesh`, or call `setInstanceMaterial`, to replace all primitives with one material.
Pass `nil` to clear that override.

`setInstancePrimitiveMaterial` overrides one primitive (0-based index).
Per-primitive overrides beat the instance-wide override.
`setInstanceColor` tints the shaded result. Default is white `{ x = 1, y = 1, z = 1 }`.

## ViewportFrame

```lua
gd3d.ViewportFrame.new(width: number, height: number) -> (ViewportFrame?, string?)
```

Creates a `CCNode` that renders its 3D scene into an off-screen buffer, then draws that texture over its content rect.
Set the node size with `setContentSize` or pass width and height to `new`.
The node uses a centered anchor point (`0.5, 0.5`), so `setPosition` places the middle of the viewport at the given point.

Camera and instances:

```lua
viewport:setCamera(transform: Transform, fovDeg: number, near: number, far: number) -> ()
viewport:getCamera() -> { transform: Transform, fovY: number, near: number, far: number }
viewport:setBackgroundColor(color: Vec3 | { r: number, g: number, b: number, a: number? }) -> ()
viewport:getBackgroundColor() -> { r: number, g: number, b: number, a: number }
viewport:setLight(direction: Vec3, color: Vec3?, intensity: number?) -> ()
viewport:setAmbient(ambient: number) -> ()
viewport:getLight() -> { direction: Vec3, color: Vec3, intensity: number, ambient: number }
viewport:addMesh(mesh: Mesh, transform: Transform, material: Material?) -> number
viewport:setInstanceMaterial(id: number, material: Material?) -> boolean
viewport:setInstancePrimitiveMaterial(id: number, primitiveIndex: number, material: Material?) -> boolean
viewport:setInstanceColor(id: number, color: Vec3) -> boolean
viewport:setInstanceTransform(id: number, transform: Transform) -> boolean
viewport:getInstanceTransform(id: number) -> Transform?
viewport:getInstanceColor(id: number) -> Vec3?
viewport:getInstanceMaterial(id: number) -> Material?
viewport:getInstancePrimitiveMaterial(id: number, primitiveIndex: number) -> Material?
viewport:getInstanceIds() -> { number }
viewport:instanceCount() -> number
viewport:removeInstance(id: number) -> boolean
viewport:clearInstances() -> ()
viewport:setCompositeEnabled(enabled: boolean) -> ()
viewport:texture() -> Texture
viewport:addDebugLine(from: Vec3, to: Vec3, color: Vec3) -> number
viewport:removeDebugLine(id: number) -> boolean
viewport:clearDebugLines() -> ()
viewport:setDebugBounds(enabled: boolean) -> ()
```

`addMesh` returns an instance id. The optional `material` sets an instance-wide override.
Setter methods return `false` for an unknown id. Readback methods return `nil`.

The camera transform is the camera world pose. The renderer uses its inverse for the view matrix.

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
Opaque and mask draws run first. Blend draws run second, sorted back to front.
`doubleSided` disables culling. Off-screen instances are frustum culled.
Opaque draws are sorted by texture and mesh. VAOs and GPU instancing are used when supported.

Reloading a mesh uploads fresh GPU data.

See [Examples](../../getting-started/examples.md) and [src/scripts/_viewportdemo.luau](../../../src/scripts/_viewportdemo.luau).

## Limits

Mesh files use the filesystem read cap.

See [Limits and errors](../cpp/limits-and-errors.md).

## Related

- [fs](fs.md)
- [Game objects](game-objects.md)
- [UI and layouts](ui.md)
- [Tasks and time](tasks.md)

## Source

- `src/bindings/render3d/Gd3dShared.hpp`
- `src/bindings/render3d/TransformBinding.cpp`
- `src/bindings/render3d/GltfBinding.cpp`
- `src/bindings/render3d/ProceduralMeshBinding.cpp`
- `src/bindings/render3d/TextureBinding.cpp`
- `src/bindings/render3d/MaterialBinding.cpp`
- `src/bindings/render3d/ViewportFrameBinding.cpp`
- `src/render3d/Transform3D.hpp`
- `src/render3d/Material.hpp`
- `src/render3d/MeshAsset.hpp`
- `src/render3d/TextureAsset.hpp`
- `src/render3d/GltfIo.hpp`
- `src/render3d/ImageDecode.hpp`
- `src/render3d/GlUtil.hpp`
- `src/render3d/CCViewportFrame.hpp`
- `src/render3d/Renderer3D.hpp`
