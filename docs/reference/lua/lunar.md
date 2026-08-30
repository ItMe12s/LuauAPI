# lunar

## Summary

`lunar` builds sprite rigs and plays keyframe animations on them.
A rig is a named hierarchy of cocos nodes.
An animation is sparse pose data that compiles to native cocos tweens.

All of `lunar` runs on the main thread. See [Getting started](../../getting-started/overview.md).
Playback runs through the normal cocos action manager,
so tweens stop when their nodes leave the scene, and tracks detach cleanly on shutdown.

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

| Field        | Type    | Default  | Notes                                                                                |
| ------------ | ------- | -------- | ------------------------------------------------------------------------------------ |
| `id`         | string  | required | Unique within the rig. Used by animations and lookups.                               |
| `sprite`     | string? | nil      | Sprite name. Without it you get an invisible container node.                         |
| `parent`     | string? | nil      | Id of an earlier node. Children move and rotate with their parent.                   |
| `x`, `y`     | number  | 0        | Position relative to the parent.                                                     |
| `rot`        | number  | 0        | Rotation in degrees.                                                                 |
| `sx`, `sy`   | number  | 1        | Scale per axis.                                                                      |
| `skx`, `sky` | number? | nil      | Skew per axis in degrees. Defaults to 0.                                             |
| `z`          | number  | 0        | Z order.                                                                             |
| `opacity`    | number? | nil      | 0 to 255, sprites only.                                                              |
| `ax`, `ay`   | number? | nil      | Anchor point per axis, 0 to 1 (not capped). Defaults to cocos' 0.5, 0.5 for sprites. |

Duplicate ids are an error.
`load` sets each created node's Geode ID to `<mod-id>/<node-id>`.
Rig lookups like `getNode` still use the plain `id`.
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

| Field       | Type    | Default  | Notes                                                                                                    |
| ----------- | ------- | -------- | -------------------------------------------------------------------------------------------------------- |
| `fps`       | number  | 30       | Frames per second. Must be above 0.                                                                      |
| `looped`    | boolean | false    | Wrap back to the start when finished.                                                                    |
| `keyframes` | table   | required | Keyed by frame number. Each value is `LunarKeyframeEntry`, node ids to poses and optional `events` list. |

Frame 12 at fps 24 is half a second.

A pose (`LunarNodePose`) sets any of `x`, `y`, `rot`, `sx`, `sy`, `skx`, `sky`, `opacity`, `z`, `ax`, `ay`, plus `easing`.
All fields are optional and missing channels hold their previous value.
The first keyframe snaps instantly instead of tweening.
Every channel except `z` tweens between keys.
Every `z` key applies instantly at its frame. Later `z` keys never tween.
`ax` and `ay` tween the anchor point per axis, so they shift how the sprite sits on its position without moving the node itself.
`skx` and `sky` tween the skew per axis in degrees.

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
lunar.animation.load(def: LunarAnimationDefTable) -> (LunarAnimationDef?, string?)
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
rig:load(spec: LunarRigSpec) -> (LunarRig?, string?)
rig:add(node: CCNode, id: string?) -> ()
rig:addTo(parentId: string, node: CCNode, id: string?) -> ()
rig:getNode(id: string) -> CCNode?
rig:listNodes() -> { string }
rig:getPose(id: string) -> LunarNodePose?
rig:loadAnimation(anim: LunarAnimationDef | LunarAnimationDefTable) -> (LunarAnimationTrack?, string?)
```

`load` applies a spec table and can be called again to add more nodes.
`add` and `addTo` attach existing cocos nodes into the rig, optionally registering an id for animations.
`getNode` returns nil for unknown or removed ids.
`listNodes` returns the live node ids, sorted alphabetically.
`getPose` reads a node's current transform into a pose table.
It returns nil for unknown or removed ids.
Channels the node type does not support, like opacity on a plain node, are left out.
The pose has no `easing` field.
`loadAnimation` accepts a def object or a raw def table and compiles it for this rig.

```lua
local arm = rig:getNode("arm")
if arm then
    arm:setScaleX(1.5)
end

for _, id in ipairs(rig:listNodes()) do
    local pose = rig:getPose(id)
    print(id, pose.x, pose.y)
end

local badge = geode.Label.create("hello")
if badge then
    rig:addTo("body", badge, "badge")
end
```

## LunarAnimationDef

```lua
def:setFps(fps: number) -> ()
def:getFps() -> number
def:setLooped(looped: boolean) -> ()
def:getLooped() -> boolean
def:addKeyframe(frame: number, nodeId: string, pose: LunarNodePose) -> ()
def:addEvent(frame: number, name: string) -> ()
def:listKeyframes() -> { { frame: number, targets: { [string]: LunarNodePose }, events: { string } } }
def:getKeyAt(frame: number) -> ({ targets: { [string]: LunarNodePose }, events: { string } })?
def:removeKeyframe(frame: number) -> boolean
def:moveKeyframe(from: number, to: number) -> boolean
```

`new` starts from fps 30, not looped, no keyframes.
Build defs by hand with `addKeyframe` or load whole tables with `lunar.animation.load`.

`listKeyframes` returns every keyframe sorted by frame.
`getKeyAt` returns the keyframe at a frame, or nil when none is there.
`removeKeyframe` deletes the keyframe at a frame and returns whether one was removed.
`moveKeyframe` moves a keyframe to another frame and returns whether one was moved.
Moving onto an occupied frame merges poses, with the moved targets winning, and appends its events.

```lua
local def = lunar.animation.new()
def:setFps(24)
def:addKeyframe(0, "arm", { rot = -25 })
def:addKeyframe(12, "arm", { rot = 25, easing = "sine_in_out" })
def:addEvent(12, "wave_done")

for _, kf in ipairs(def:listKeyframes()) do
    print(kf.frame, kf.events)
end

local track = rig:loadAnimation(def)
```

## LunarAnimationTrack

A track is one compiled animation playing on one rig.
Keep a Luau reference to it while it runs. Dropping the last reference stops the tweens.

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
track:sample(t: number) -> { [string]: LunarNodePose }
track:seek(t: number) -> ()
track:currentTime() -> number
```

- `play` restarts from time zero. It warns and does nothing on an empty animation.
- `pause` freezes playback. `unpause` resumes it.
- `stop` halts and rewinds. The next `play` starts over.
- `setSpeed` takes effect immediately, even mid-tween.
  Speed must be above 0. Zero or negative speeds raise an error.
- `duration` is the animation length in seconds, independent of speed.
- `bindEvent` registers `fn` to run whenever an event with that name fires.
  Multiple fns per name are allowed, and every bound fn fires once per matching marker.
  Markers already passed before `play` or `unpause` are skipped.

Observing events:

```lua
local track = rig:loadAnimation(Wave)
if not track then return end

track:bindEvent("step", function(name)
    print("marker fired:", name)
end)
track:play()
```

When a looped track reaches its end it wraps and keeps playing.
Tracks stop cleanly when the runtime shuts down.

`sample` reads poses at a time without moving the playhead, and writes them onto the rig.
`seek` jumps the playhead to a time.
On a playing track it relaunches from that time, so seeked events fire.
On a paused or stopped track it writes the sampled pose directly.
Events at or before the seek target are skipped.
`currentTime` is the playhead in seconds, independent of speed.
Seeking past the end lands on the last frame.

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

## Lunar Animator

The mod has a built-in sprite rig and animation editor, the **Lunar Animator**.
It is developer mode only and opens from the bottom menu button.

It edits the same rig and animation files documented here.
Files it writes load back through `rig:load` and `lunar.animation.load`.
You can add the same files to your own mod (kinda like exporting).

## Example

Full round trip with separate files:

```lua
-- Main script.
local Robot = require("./Robot.rig")
local Wave = require("./Wave.anim")
local modId = geode.Mod.getID()

local rig = lunar.rig.new():load(Robot)
if not rig then return end

rig:setID(modId .. "/robot-rig")
local director = geode.cocos2d.CCDirector.sharedDirector()
if not director then return end

local winSize = director:getWinSize()
rig:setPosition({ x = winSize.width / 2, y = winSize.height / 2 })
anyLayer:addChild(rig) -- Any node you own that stays on screen.

local track = rig:loadAnimation(Wave)
if not track then return end

track:setSpeed(2) -- Twice as fast, even mid-play.
track:play()
_G.robotDemo = { rig = rig, track = track }
```

## Limits

`fps` and track `speed` must be above 0.
A bad `fps` fails the load and `setSpeed` errors on zero or negative.
Anchor values are not capped.
Other caps live in [Limits and errors](../cpp/limits-and-errors.md).

## Related

- [cocos](cocos.md)
- [game objects](game-objects.md)
- [modules](modules.md)
- [type stubs](type-stubs.md)
- [globals](globals.md)
- [Getting started](../../getting-started/overview.md)

## Source

- `src/bindings/lunar/LunarBinding.cpp`
- `src/bindings/lunar/LunarRig.cpp`
- `src/bindings/lunar/LunarAnimation.cpp`
- `src/bindings/lunar/LunarModel.cpp`
