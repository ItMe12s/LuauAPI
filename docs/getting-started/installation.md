# Installation

## Summary

How to build a Geode mod that uses LuauAPI.
Install the mod, declare a dependency, and pack your `.luau` files as resources.
No build steps are needed for this path.

## Install the mod

Install LuauAPI from the Geode index in-game, or grab the `.geode` file from the GitHub release tab.
LuauAPI ships with the mod id `imes.luauapi`.
It loads early with first priority, so the runtime is ready for other mods as soon as the game starts.

## Depend on it

If you do not have a mod yet, create one with the Geode CLI (`geode new`).
Then add a dependency on `imes.luauapi` in your `mod.json` and declare your script files as resources.

```json
{
    "dependencies": {
        "imes.luauapi": ">=0.1.0-beta.1"
    },
    "resources": {
        "files": [
            "mod/*.luau",
            "assets/*.glb",
            "assets/*.gltf"
        ]
    }
}
```

Put your `.luau` files under the resources path you declare.
They get packed with your mod and load from your mod resources directory at runtime.
Ship 3D assets the same way. Pack `.glb` or `.gltf` files under `"resources"` and load them with `gd3d.gltf.loadMesh`.
See [gd3d](../reference/lua/gd3d.md).

## Settings

LuauAPI ships two settings, both off by default:

- `enable-developer-mode` turns on the built-in developer tools.
- `enable-executor` shows the script executor. It requires developer mode.

## Supported platforms

- Windows
- macOS (arm64 and x86_64)
- iOS (arm64)
- Android (32-bit and 64-bit)

## Next

- [Your first script](first-script.md)
- [Editor setup](editor-setup.md)

## Related

- [Overview](overview.md)
- [Building from source](../contributor/building.md)

## Source

- `mod.json`
- `CMakeLists.txt`
