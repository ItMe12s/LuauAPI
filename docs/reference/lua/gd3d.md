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
    addMesh: (self: ViewportFrame, mesh: Mesh, transform: Transform, material: Material?) -> number,
    setInstanceMaterial: (self: ViewportFrame, id: number, material: Material?) -> boolean,
    setInstanceColor: (self: ViewportFrame, id: number, color: Vec3) -> boolean,
    setInstanceTransform: (self: ViewportFrame, id: number, transform: Transform) -> boolean,
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
transform * otherTransform -> Transform
```

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

Mesh methods:

```lua
mesh:materialCount() -> number
mesh:getMaterial(index: number) -> Material?
```

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
viewport:addMesh(mesh: Mesh, transform: Transform, material: Material?) -> number
viewport:setInstanceMaterial(id: number, material: Material?) -> boolean
viewport:setInstanceColor(id: number, color: Vec3) -> boolean
viewport:setInstanceTransform(id: number, transform: Transform) -> boolean
viewport:removeInstance(id: number) -> boolean
viewport:clearInstances() -> ()
```

`addMesh` returns an instance id you pass to the other instance methods.
The optional `material` argument sets an instance-wide material override at creation time.
`setInstanceTransform` and `removeInstance` return `false` when the id is unknown.

The camera transform is the camera's world pose.
The renderer uses its inverse for the view matrix and a vertical field-of-view perspective projection.

## Example

Load the sample donut mesh, spin it in a viewport, and attach the viewport to the menu layer.
See [src/scripts/_viewportdemo.luau](../../../src/scripts/_viewportdemo.luau) for the full script.

```lua
local Transform = gd3d.Transform

local mesh, err = gd3d.gltf.loadMesh("resources", "test_donut.glb")
if not mesh then
    print(err)
    return
end

local vp = gd3d.ViewportFrame.new(300, 300)
vp:setCamera(Transform.new({ x = 0, y = 1, z = 4 }, { x = 0, y = 0, z = 0 }), 70, 0.1, 100)

local id = vp:addMesh(mesh, Transform.new())
vp:setInstanceColor(id, { x = 1, y = 0.85, z = 0.7 })
local angle = 0

task.every(1 / 60, function()
    angle += 0.02
    vp:setInstanceTransform(id, Transform.fromEuler(0.4, angle, 0))
end)

-- ViewportFrame is a CCNode. Parent it under any layer or popup.
local menu = geode.gd.MenuLayer.get()
if menu then
    vp:setPosition({ x = 200, y = 200 })
    menu:addChild(vp)
end
```

## Limits

Mesh files use the same read cap as [fs](fs.md) (`32 MiB`).
External glTF buffer files must stay inside the sandbox root.

See [Limits and errors](../cpp/limits-and-errors.md) for caps and error strings.

## Related

- [fs](fs.md)
- [Game objects](game-objects.md)
- [UI and layouts](ui.md)
- [Tasks and time](tasks.md)
- [Limits and errors](../cpp/limits-and-errors.md)

## Source

- `src/bindings/render3d/TransformBinding.cpp`
- `src/bindings/render3d/GltfBinding.cpp`
- `src/bindings/render3d/MaterialBinding.cpp`
- `src/bindings/render3d/ViewportFrameBinding.cpp`
- `src/render3d/Transform3D.hpp`
- `src/render3d/MeshAsset.hpp`
- `src/render3d/CCViewportFrame.hpp`
- `src/render3d/Renderer3D.hpp`
