# gd3d

## Summary

`gd3d` provides lightweight 3D rendering inside the cocos2d scene graph.
Load glTF meshes, place them in a `ViewportFrame` node, and attach that node like any other `CCNode`.

The namespace has four parts:

- `gd3d.Transform` for position and rotation
- `gd3d.gltf` for loading mesh assets
- `gd3d.Material` for solid-color or glTF-derived materials
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

Transforms are immutable. `withPosition` and `withRotation` return new values without mutating the receiver.
`eulerAngles` returns pitch, yaw, and roll in radians (same convention as `fromEuler`).

## glTF loading

```lua
gd3d.gltf.loadMesh(root: FsRoot, path: string) -> (Mesh?, string?)
```

Loads a glTF 2.0 file (`.glb` or `.gltf`) from a mod sandbox root.
The first argument uses the same roots as [fs](fs.md): `"save"`, `"config"`, `"persistent"`, or `"resources"`.
Shipped assets belong under `"resources"`.

Returns a mesh handle, or `nil` and an error message.
Mesh data is released when the handle is collected and no viewport instance uses it anymore.
Viewport instances added with `addMesh` keep the mesh alive on their own,
so you do not need to keep the handle around after adding.

Supported content:

- Embedded and external buffers inside the sandbox root
- Triangle primitives only
- Node world transforms baked into vertex positions and normals
- glTF materials: `pbr_metallic_roughness.base_color_factor`
- Base color textures (PNG or JPEG), embedded in GLB or referenced inside the sandbox root
- `TEXCOORD_0` on primitives that use a base color texture

Not supported:

- Draco compression
- meshopt compression
- Sparse accessors
- Metallic/roughness maps, normal maps, emissive textures
- Alpha blending (`alphaMode` blend)
- KHR texture extensions (BasisU, WebP, and similar)

Primitives with a textured material must include `TEXCOORD_0`, loading fails otherwise.

The loader walks every glTF node that has a mesh and merges them into one asset.
Node transforms are baked into vertex positions and normals.
It does not preserve the scene hierarchy, skeletal animation, morph targets, or glTF cameras and lights.
Loading the same file again creates a new mesh handle and uploads GPU data again.

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

## Material

```lua
gd3d.Material.new({ color: Vec3 | { r: number, g: number, b: number, a: number? } }) -> Material
```

Creates a solid-color material with no texture.
Use `{ r, g, b, a }` for RGBA, or a `Vec3` `{ x, y, z }` for opaque RGB.

Instance methods:

```lua
material:baseColor() -> { r: number, g: number, b: number, a: number }
material:hasTexture() -> boolean
```

`hasTexture` is `true` for materials returned by `mesh:getMaterial` when the glTF material references a base color texture.

### Instance override

Each viewport instance draws with glTF materials by default, every primitive uses its own `materialIndex` from the file.
Pass an optional fourth argument to `addMesh`, or call `setInstanceMaterial`,
to replace all primitives in that instance with one material.
Pass `nil` to `setInstanceMaterial` to clear the override and restore per-primitive glTF materials.
Use `setInstancePrimitiveMaterial` to override a single primitive by index (0-based, matching `mesh:primitiveCount()`).
Per-primitive overrides take priority over the instance-wide override.
Pass `nil` to clear a per-primitive override.
`setInstanceColor` multiplies the final shaded color (material albedo times lighting) by a per-instance RGB tint.
Default tint is white `{ x = 1, y = 1, z = 1 }`.

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
```

`addMesh` returns an instance id you pass to the other instance methods.
The optional `material` argument sets an instance-wide material override at creation time.
`setInstanceMaterial`, `setInstancePrimitiveMaterial`, `setInstanceColor`, `setInstanceTransform`, and `removeInstance` return `false` when the id is unknown.

Readback methods return `nil` for an unknown id.
`getInstanceMaterial` returns the instance-wide override set by `addMesh` or `setInstanceMaterial`,
or `nil` when no override is active (glTF per-primitive materials are not returned).
`getInstancePrimitiveMaterial` returns the per-primitive override for that index, or `nil` when none is set.
`getInstanceIds` returns all instance ids in ascending order.
`instanceCount` is the number of active instances.

The camera transform is the camera's world pose.
The renderer uses its inverse for the view matrix and a vertical field-of-view perspective projection.

Background color clears the off-screen buffer before drawing instances.
Use `{ r, g, b, a }` for RGBA, or a `Vec3` `{ x, y, z }` for opaque RGB.
Default is fully transparent `{ r = 0, g = 0, b = 0, a = 0 }`.
When alpha is less than 1, the 2D scene behind the viewport shows through.

Lighting uses a fixed Lambert shader with one directional light plus ambient fill.
`setLight` sets the light direction (normalized at render time), RGB color, and intensity multiplier.
Omit `color` for white `{ x = 1, y = 1, z = 1 }` and `intensity` for `1`.
A zero-length direction raises an error.
`setAmbient` clamps its argument to the range `[0, 4]`.
`getLight` returns the current direction, color, intensity, and ambient values.
Defaults match the built-in look: direction `{ x = 0.35, y = 0.85, z = 0.4 }`, white light, intensity `1`, ambient `0.15`.

### Rendering model

Drawing needs an active OpenGL context from the game.
If the context is not ready yet, framebuffer setup and draw calls are skipped until it is.

The viewport renders into an off-screen buffer sized in pixels using the cocos content scale factor,
then blits that texture into the scene graph.

Shading is a fixed Lambert pass with configurable light direction, color, intensity, and ambient level.
There is no alpha blending. Instances draw opaque.

Reloading a mesh file or creating a new handle uploads fresh GPU data even when the path is the same.

See [Examples](../../getting-started/examples.md).
See [src/scripts/_viewportdemo.luau](../../../src/scripts/_viewportdemo.luau) for a longer demo with color cycling.

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
- `src/bindings/render3d/MaterialBinding.cpp`
- `src/bindings/render3d/ViewportFrameBinding.cpp`
- `src/render3d/Transform3D.hpp`
- `src/render3d/Material.hpp`
- `src/render3d/MeshAsset.hpp`
- `src/render3d/GltfIo.hpp`
- `src/render3d/ImageDecode.hpp`
- `src/render3d/GlUtil.hpp`
- `src/render3d/CCViewportFrame.hpp`
- `src/render3d/Renderer3D.hpp`
