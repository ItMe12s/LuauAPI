# Reference: type stubs and editor setup

## Summary

The build generates Luau type stubs for the game bindings. These stubs provide autocomplete and type checks in your editor. This page lists the stubs and the editor settings. For a step by step walkthrough, see [Editor setup](../../getting-started/editor-setup.md).

## What the stubs are

The code generator writes `.d.luau` files into the `types/` folder. They describe the bound classes and the `geode` namespace. They are generated, so you do not edit them by hand.

The generated files are:

- `types/geode.d.luau`: the `geode` namespace root, along with `HookHandle` and `HookCallbackTable`.
- `types/geode_cocos2d.d.luau` and `types/geode_cocos2d_factories.d.luau`: the engine classes.
- `types/geode_gd.d.luau`, `types/geode_gd_2.d.luau`, `types/geode_gd_3.d.luau`, `types/geode_gd_4.d.luau`: the Geometry Dash classes, split across files.
- `types/geode_gd_factories.d.luau`: the Geometry Dash factory functions.

The `task` and `time` stubs come from `tools/luau_codegen/extra_bindings/task.dluau`.

## Editor setup

The repository ships editor configuration for the Luau language server.

`.luaurc` enables strict type checking and adds a `types` alias:

```json
{
    "languageMode": "strict",
    "aliases": {
        "types": "types"
    }
}
```

Configure `.vscode/settings.json` to register each stub file and ignore `.d.luau` files. The order in `definitionFiles` is important.
Factories must come before their namespace class files, and cocos files before gd files. Do not change this order.

```json
{
    "luau-lsp.platform.type": "standard",
    "luau-lsp.types.definitionFiles": {
        "@geode-cocos2d-factories": "types/geode_cocos2d_factories.d.luau",
        "@geode-cocos2d": "types/geode_cocos2d.d.luau",
        "@geode-gd-factories": "types/geode_gd_factories.d.luau",
        "@geode-gd": "types/geode_gd.d.luau",
        "@geode-gd-2": "types/geode_gd_2.d.luau",
        "@geode-gd-3": "types/geode_gd_3.d.luau",
        "@geode-gd-4": "types/geode_gd_4.d.luau",
        "@geode": "types/geode.d.luau"
    },
    "luau-lsp.ignoreGlobs": [
        "**/*.d.luau"
    ]
}
```

## Regenerating the stubs

The stubs are written during the build by the `luauapi_codegen` target. To refresh them, build the project. See [Building](../../getting-started/building.md) and [Codegen](../../dev/codegen.md).

## Source

- `.luaurc`
- `.vscode/settings.json`
- `tools/luau_codegen/emit_luau_types.py`
- `tools/luau_codegen/extra_bindings/task.dluau`
- `CMakeLists.txt`
