# Setup your visual studio

You would want autocomplete and help from the lsp.

Use the `Luau Language Server` extension by JohnnyMorganz.

## Getting types

To get the `.d.luau` files, compile this mod on your pc.
Then copy the `types` folder into your mod project root.

## Setting files

Add this to your `.vscode/settings.json`

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

**YourMod**
|- .vscode
|   |- settings.json
|
|- mod
|   |- Bootstrap.lua
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
