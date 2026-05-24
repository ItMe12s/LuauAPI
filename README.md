# Setup your visual studio

You would want autocomplete and help from the lsp.
Use the `Luau Language Server` extension by JohnnyMorganz.

## Getting types

To get the `.d.luau` files, compile this mod on your pc.
Then copy the `types` folder into your mod project root.

**BTW IT TAKES LIKE FOREVER TO COMPILE**, if you don't have a workstation cpu.

Also do the vscode `>Developer: Restart Extension Host` after doing stuff with luau lsp.

## Setting files

Add this to your `.vscode/settings.json`
It's split into 3 files because luau lsp can't just process 13k lines at once.

```json
{
    "luau-lsp.platform.type": "standard",
    "luau-lsp.types.definitionFiles": {
        "@geode-cocos2d": "types/geode_cocos2d.d.luau",
        "@geode-gd": "types/geode_gd.d.luau",
        "@geode": "types/geode.d.luau"
    },
    "luau-lsp.ignoreGlobs": [
        "**/*.d.luau"
    ]
}
```

At your project root, add this `.luaurc` file

```json
{
    "languageMode": "strict",
    "aliases": {
        "types": "types"
    }
}
```

## What it should look like

Since you can't do folders inside resources, you'll just have to name and prefix them properly.

```.
YourMod
|- .vscode
|   |- settings.json
|
|- mod
|   |- Bootstrap.luau
|   |- mod_YourModule.luau
|   |- hook_GameLayer.luau
|   |- CoolScript.luau
|
|- src
|   |- main.cpp
|
|- types
|   |- geode_cocos2d.d.luau
|   |- geode_gd.d.luau
|   |- geode.d.luau
|
|- .luaurc
|
|... and everything else from the Geode mod template.
```

## Don't forget to include the files

Add `Bootstrap.luau` to your `mod.json` resources so Geode packages it as a runtime file:

```json
{
    "resources": {
        "files": [
            "mod/*.luau"
        ]
    }
}
```

`runFile(Mod::get()->getResourcesDir(), "Bootstrap.luau")` expects the script as a flat resource name.
