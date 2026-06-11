# gd3d

## Summary

`gd3d` provides lightweight 3D rendering inside the cocos2d scene graph.
Load glTF meshes, place them in a `ViewportFrame` node, and attach that node like any other `CCNode`.

The namespace has three parts:

- `gd3d.Transform` for position and rotation
- `gd3d.gltf` for loading mesh assets
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

type Mesh = {
    vertexCount: (self: Mesh) -> number,
    primitiveCount: (self: Mesh) -> number,
    boundingBox: (self: Mesh) -> { min: Vec3, max: Vec3, empty: boolean },
}

type ViewportFrame = CCNode & {
    setCamera: (self: ViewportFrame, transform: Transform, fovDeg: number, near: number, far: number) -> (),
    getCamera: (self: ViewportFrame) -> { transform: Transform, fovY: number, near: number, far: number },
    addMesh: (self: ViewportFrame, mesh: Mesh, transform: Transform) -> number,
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
Drop the handle or call `collectgarbage()` to release CPU and GPU data when you are done.

Supported content:

- Embedded and external buffers inside the sandbox root
- Triangle primitives only
- Node world transforms baked into vertex positions and normals

Not supported:

- Draco compression
- meshopt compression
- Sparse accessors

## ViewportFrame

```lua
gd3d.ViewportFrame.new(width: number, height: number) -> (ViewportFrame?, string?)
```

Creates a `CCNode` that renders its 3D scene into an off-screen buffer, then draws that texture over its content rect.
Set the node size with `setContentSize` or pass width and height to `new`.

Camera and instances:

```lua
viewport:setCamera(transform: Transform, fovDeg: number, near: number, far: number) -> ()
viewport:getCamera() -> { transform: Transform, fovY: number, near: number, far: number }
viewport:addMesh(mesh: Mesh, transform: Transform) -> number
viewport:setInstanceTransform(id: number, transform: Transform) -> boolean
viewport:removeInstance(id: number) -> boolean
viewport:clearInstances() -> ()
```

`addMesh` returns an instance id you pass to `setInstanceTransform` and `removeInstance`.
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
- `src/bindings/render3d/ViewportFrameBinding.cpp`
- `src/render3d/Transform3D.hpp`
- `src/render3d/MeshAsset.hpp`
- `src/render3d/CCViewportFrameNode.hpp`
- `src/render3d/Renderer3D.hpp`
