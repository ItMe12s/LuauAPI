# Installation

## Summary

How to add LuauAPI to your own Geode mod. Install the mod, declare a dependency, and pack your
`.luau` files as resources. No build steps are needed for this path.

## Install the mod

Install LuauAPI from the Geode index in-game, or grab the `.geode` file from the GitHub release
tab. LuauAPI ships with the mod id `imes.luauapi`. It loads early with first priority, so the
runtime is ready for other mods as soon as the game starts.

## Depend on it

Add a dependency on `imes.luauapi` in your mod and declare your script files as resources.

```json
{
    "dependencies": {
        "imes.luauapi": ">=0.1.0-beta.1"
    },
    "resources": {
        "files": [
            "mod/*.luau"
        ]
    }
}
```

Put your `.luau` files under the resources path you declare. They get packed with your mod and
load from your mod resources directory at runtime.

## Settings

LuauAPI ships two settings, both off by default:

- `enable-developer-mode` turns on the built-in developer tools.
- `enable-executor` shows the script executor. It requires developer mode.

## Supported platforms

LuauAPI supports Windows, macOS (arm64 and x86_64), iOS (arm64), and Android (32-bit and 64-bit).

## Next

- [Your first script](first-script.md)
- [Editor setup](editor-setup.md)

## Related

- [Overview](overview.md)
- [Building from source](../contributor/building.md)

## Source

- `mod.json`
- `CMakeLists.txt`
