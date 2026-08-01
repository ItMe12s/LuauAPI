# Installation

## Summary

The fastest way to start a LuauAPI mod is the example template.
Create it with the Geode CLI or download it from GitHub.
For an existing mod, add the dependency and resources manually.
You do not need to build LuauAPI from source.
See the [README](../README.md) beta note for release stability.

## Start a new mod from the template

The [LuauAPI example mod template](https://github.com/ItMe12s/luauapi-example-mod)
already contains the dependency, resources, editor config, and first script.

### Geode CLI

This is the recommended path and the easiest.

1. Run:

   ```sh
   geode new
   ```

2. Select `GitHub Repository`.
3. Enter `ItMe12s/luauapi-example-mod` at the `Template:` prompt.
4. Finish the project prompts. The CLI fills the template metadata.

### Download ZIP

1. Visit the [template repository](https://github.com/ItMe12s/luauapi-example-mod).
2. Select the green `Code` button, then `Download ZIP`.
3. Extract the archive and rename the project folder.
4. Replace every `$GEODE_VERSION` and `$MOD_*` placeholder in `mod.json`.
   Use `5.8.2` for `$GEODE_VERSION`.

## Install the mod

Install LuauAPI from [GitHub releases](https://github.com/ItMe12s/LuauAPI/releases) for the latest features and patches,
or from the Geode in-game mod index when it is listed there (more stable and approved by index staff).
Download the `.geode` file and place it in your mods folder if you install manually.
LuauAPI ships with the mod id `imes.luauapi`.
It loads early with first priority, so the runtime is ready for other mods as soon as the game starts.

Requires Geode **5.8.2** or newer (see `mod.json` `"geode"` field).

## Finish the template setup

For either template path:

1. Check that `mod.json` contains your real mod metadata with no `$GEODE_VERSION` or `$MOD_*` placeholders.
2. Update `logo.png`, `about.md`, `changelog.md`, and `support.md`.
3. Follow [Editor setup](editor-setup.md) to download `types/geode.d.luau`.
4. Build the project:

```sh
geode build
```

The template's `src/main.cpp` runs `mod/Bootstrap.luau` when the mod loads.

## Add LuauAPI to an existing mod

Add a dependency on `imes.luauapi` in your `mod.json` and declare your script files as resources.

```json
{
    "dependencies": {
        "imes.luauapi": ">=0.1.0-beta.1"
    },
    "resources": {
        "files": [
            "mod/**/*.luau",
            "assets/*.glb",
            "assets/*.gltf",
            "assets/*.ttf"
        ]
    }
}
```

Put your `.luau` files under the resources path you declare.
Geode packs them into a flat resources folder at runtime.
There are no subdirectories. Use unique file names.
Subfolders in your repo, such as `mod/luauapi/`, are for organization only.
They load from your mod resources directory at runtime.
Ship 3D assets the same way. Pack `.glb` or `.gltf` files under `"resources"` and load them with `gd3d.gltf.loadMesh`.
Include `.ttf` files when you use custom ImGui fonts. See [gd3d](../reference/lua/gd3d.md) and [imgui](../reference/lua/imgui.md).

## Run from C++

Most mods only need the dependency above.
If your mod already has C++ entry points, call `imes::luauapi::runFile` or `runScript` with your resources directory.
See [Getting started](overview.md) for the main-thread rule, [Your first script](first-script.md),
and the [C++ API reference](../reference/cpp/api-reference.md).
To publish typed C++ functions or values to Luau, see [Native C++ registration](../reference/cpp/native-registration.md).

## Supported platforms

- Windows
- macOS (arm64 and x86_64)
- iOS (arm64)
- Android (32-bit and 64-bit)

## Next

- [Editor setup](editor-setup.md)
- [Your first script](first-script.md)

## Related

- [Getting started](overview.md)
- [Editor setup](editor-setup.md)
- [Your first script](first-script.md)
- [Native C++ registration](../reference/cpp/native-registration.md)
- [Examples](examples.md)
- [LuauAPI mod guidelines](../mod_guidelines.md)
- [Building from source](../contributor/building.md)

## Source

- `mod.json`
- `CMakeLists.txt`
