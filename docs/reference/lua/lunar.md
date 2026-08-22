# lunar

## Summary

`lunar` builds sprite rigs and plays keyframe animations on them.
A rig is a named hierarchy of cocos nodes. An animation is sparse pose data that compiles to native cocos tweens.

All of `lunar` runs on the main thread. See [Getting started](../../getting-started/overview.md).
Playback uses standard cocos actions, so tweens pause with the scene and clean up on shutdown.

```lua
local modId = geode.Mod.getID()

local rig = lunar.rig.new():load({
    nodes = {
        { id = "body", sprite = modId .. "/TestSprite1.png" },
        { id = "arm", sprite = modId .. "/TestSprite2.png", parent = "body", x = 70 },
    },
})

local wave = lunar.animation.load({
    fps = 12,
    looped = true,
    keyframes = {
        [0] = { arm = { rot = -25 } },
        [6] = { arm = { rot = 25, easing = "sine_in_out" } },
        [12] = { arm = { rot = -25, easing = "sine_in_out" } },
    },
})

local track = rig:loadAnimation(wave)
track:play()
```

See [mod/demo/demo_lunar.luau](../../../mod/demo/demo_lunar.luau) for a runnable version.

## Rig files

A rig file is a plain Luau module that returns a spec table (`LunarRigSpec`).
The convention is one `.rig.luau` file per rig:

```lua
-- Robot.rig.luau
return {
    nodes = {
        { id = "body", sprite = "body.png" },
        { id = "arm", sprite = "arm.png", parent = "body" },
    },
}
```

Each entry in `nodes` becomes one cocos node:

| Field      | Type    | Default  | Notes                                                                                             |
| ---------- | ------- | -------- | ------------------------------------------------------------------------------------------------- |
| `id`       | string  | required | Unique within the rig. Used by animations and lookups.                                            |
| `sprite`   | string? | nil      | Sprite name. Without it you get an invisible container node.                                      |
| `parent`   | string? | nil      | Id of an earlier node. Children move and rotate with their parent.                                |
| `x`, `y`   | number  | 0        | Position relative to the parent.                                                                  |
| `rot`      | number  | 0        | Rotation in degrees.                                                                              |
| `sx`, `sy` | number  | 1        | Scale per axis.                                                                                   |
| `z`        | number  | 0        | Z order.                                                                                          |
| `opacity`  | number? | nil      | 0 to 255, sprites only.                                                                           |
| `ax`, `ay` | number? | nil      | Anchor point per axis, 0 to 1 (values outside work too). Defaults to cocos' 0.5, 0.5 for sprites. |

Duplicate ids are an error.
`load` gives each created node a string id of `<mod-id>/<node-id>` automatically.
A missing sprite loads the node with Geode's checkerboard fallback texture.
The load logs a warning, and the rest of the rig still works.

## Sprite names

Rigs load sprites through the Geode resource system.

- Names resolve as files first, then as frames from GD's atlases.
- Your own art must use the prefixed form `"<mod-id>/name.png"`, exactly like the `_spr` prefix in C++.
- Loose PNGs declared under `resources.sprites` load by file path.
- Sprites declared under `resources.spritesheets` load by frame name.
  Sheets are packed by the CLI and are the better choice at scale.

```lua
local SPR = geode.Mod.getID() .. "/"
local spec = {
    nodes = {
        { id = "body", sprite = SPR .. "robot.png" },
    },
}
```

If a frame is not found, the node still loads and shows Geode's checkerboard fallback texture.
The log names the prefix rule:

```text
rig node 'body': sprite 'robot.png' not found, mod sprites must be prefixed '<mod-id>/name.png'
```

## Animation files

An animation file is also a plain Luau module.
The convention is one `.anim.luau` file per animation.
It returns a def table (`LunarAnimationDefTable`):

```lua
-- Wave.anim.luau
return {
    fps = 30,
    looped = false,
    keyframes = {
        [0] = { arm = { rot = -25 } },
        [12] = { arm = { rot = 25, easing = "cubic_in_out" } },
    },
}
```

| Field       | Type    | Default  | Notes                                                                                    |
| ----------- | ------- | -------- | ---------------------------------------------------------------------------------------- |
| `fps`       | number  | 30       | Frames per second. Frame 12 at fps 24 is half a second. Must be above 0.                 |
| `looped`    | boolean | false    | Wrap back to the start when finished.                                                    |
| `keyframes` | table   | required | Keyed by frame number. Each value maps node ids to poses and may carry an `events` list. |

A pose (`LunarNodePose`) sets any of `x`, `y`, `rot`, `sx`, `sy`, `opacity`, `z`, `ax`, `ay`, plus `easing`.
All fields are optional and missing channels hold their previous value.
The first keyframe snaps instantly instead of tweening.
Every channel except `z` tweens between keys. `z` always applies instantly.
`ax` and `ay` tween the anchor point per axis, so they shift how the sprite sits on its position without moving the node itself.

A keyframe entry can also carry animation events. Set `events` to one name or an array of names.
Events do nothing on their own. A playing track fires an event when the playhead reaches its frame.
Event-only keyframes still count toward the animation length. Events fire again on every loop.

```lua
keyframes = {
    [12] = { arm = { rot = 25 } },
    [67] = { events = { "particle_thingy", "sound1" } },
},
```

Animations are decoupled from rigs.
They target node ids, so one animation can drive any rig with matching ids.
Unknown ids are skipped with a warning at play time.

## Functions

```lua
lunar.rig.new() -> LunarRig
lunar.animation.new() -> LunarAnimationDef
lunar.animation.load(def: LunarAnimationDefTable) -> LunarAnimationDef?
```

`animation.load` parses and validates a def table right away.
`lunar.animation.load`, `rig:load`, and `rig:loadAnimation`
return `nil` plus an error message on a bad def or spec table:

```lua
local wave, err = lunar.animation.load(def)
if not wave then
    print("bad animation:", err)
end
```

## LunarRig

`LunarRig` extends `CCNode`, so all node methods work on it.

```lua
rig:load(spec: LunarRigSpec) -> LunarRig?
rig:add(node: CCNode, id: string?) -> ()
rig:addTo(parentId: string, node: CCNode, id: string?) -> ()
rig:getNode(id: string) -> CCNode?
rig:loadAnimation(anim: LunarAnimationDef | LunarAnimationDefTable) -> LunarAnimationTrack?
```

`load` applies a spec table and can be called again to add more nodes.
`add` and `addTo` attach existing cocos nodes into the rig, optionally registering an id for animations.
`getNode` returns nil for unknown or removed ids.
`loadAnimation` accepts a def object or a raw def table and compiles it for this rig.

## LunarAnimationDef

```lua
def:setFps(fps: number) -> ()
def:getFps() -> number
def:setLooped(looped: boolean) -> ()
def:getLooped() -> boolean
def:addKeyframe(frame: number, nodeId: string, pose: LunarNodePose) -> ()
def:addEvent(frame: number, name: string) -> ()
```

`new` starts from fps 30, not looped, no keyframes.
Build defs by hand with `addKeyframe` or load whole tables with `lunar.animation.load`.

## LunarAnimationTrack

A track is one compiled animation playing on one rig.
Keep a Luau reference to it while it runs, since dropping the last reference stops the tweens.

```lua
track:play() -> ()
track:pause() -> ()
track:unpause() -> ()
track:stop() -> ()
track:setSpeed(speed: number) -> ()
track:isPlaying() -> boolean
track:isPaused() -> boolean
track:speed() -> number
track:duration() -> number
track:bindEvent(name: string, fn: (eventName: string) -> ()) -> ()
```

- `play` restarts from time zero. It warns and does nothing on an empty animation.
- `pause` freezes playback. `unpause` resumes it.
- `stop` halts and rewinds. The next `play` starts over.
- `setSpeed` takes effect immediately, even mid-tween.
- `duration` is the animation length in seconds, independent of speed.
- `bindEvent` registers `fn` to run whenever an event with that name fires.
  Multiple fns per name are allowed, and every bound fn fires once per matching marker.
  Markers already passed before `play` or `unpause` are skipped.

When a looped track reaches its end it wraps and keeps playing.
Tracks stop cleanly when the runtime shuts down.

## Easing

Easing is per pose. Every tweened channel in one pose shares the same curve.

| Names                                         | Family         |
| --------------------------------------------- | -------------- |
| `linear`                                      | Constant speed |
| `quad_in`, `quad_out`, `quad_in_out`          | Power 2        |
| `cubic_in`, `cubic_out`, `cubic_in_out`       | Power 3        |
| `quart_in`, `quart_out`, `quart_in_out`       | Power 4        |
| `quint_in`, `quint_out`, `quint_in_out`       | Power 5        |
| `sine_in`, `sine_out`, `sine_in_out`          | Smooth sine    |
| `expo_in`, `expo_out`, `expo_in_out`          | Exponential    |
| `back_in`, `back_out`, `back_in_out`          | Overshoot      |
| `elastic_in`, `elastic_out`, `elastic_in_out` | Elastic wobble |
| `bounce_in`, `bounce_out`, `bounce_in_out`    | Bounce         |

An unknown easing string fails the load with the offending name in the message.

## Example

Full round trip with separate files:

```lua
-- Main script.
local Robot = require("./Robot.rig")
local Wave = require("./Wave.anim")
local modId = geode.Mod.getID()

local rig = lunar.rig.new():load(Robot)
rig:setID(modId .. "/robot-rig")
local director = geode.cocos2d.CCDirector.sharedDirector()
local winSize = director:getWinSize()
rig:setPosition({ x = winSize.width / 2, y = winSize.height / 2 })
anyLayer:addChild(rig) -- Any node you own that stays on screen.

local track = rig:loadAnimation(Wave)
track:setSpeed(2) -- Twice as fast, even mid-play.
track:play()
_G.robotDemo = { rig = rig, track = track }
```

## Related

- [Getting started](../../getting-started/overview.md)
- [cocos](cocos.md)
- [game objects](game-objects.md)
- [modules](modules.md)
- [globals](globals.md)
- [type stubs](type-stubs.md)

## Source

- `src/bindings/lunar/LunarBinding.cpp`
- `src/bindings/lunar/LunarRig.cpp`
- `src/bindings/lunar/LunarAnimation.cpp`
- `src/bindings/lunar/LunarModel.cpp`
