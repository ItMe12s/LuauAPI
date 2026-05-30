# Reference: type stubs and editor setup

## Summary

The build generates Luau type stubs for the game bindings. These stubs provide autocomplete and type checks in your editor. This page lists the stubs and the editor settings. For a step by step walkthrough, see [Editor setup](../../getting-started/editor-setup.md).

## What the stubs are

The code generator writes `.d.luau` files into the `types/` folder. They describe the bound classes and the `geode` namespace. They are generated, so you do not edit them by hand.

The generator writes one file per class so that no single file is large enough to trip luau-lsp's "Code is too complex to typecheck" error.
The files are:

- `types/geode_cocos2d_<Class>.d.luau`: one stub per cocos2d class.
- `types/geode_gd_<Class>.d.luau`: one stub per Geometry Dash class.
- `types/geode_cocos2d_factories.d.luau` and `types/geode_gd_factories.d.luau`: the static-method factories and the enum aliases for each namespace.
- `types/geode.d.luau`: the `geode` namespace root, along with `HookHandle` and `HookCallbackTable`.
- `types/luau-lsp.json`: a generated JSON array listing every file above in load order. Use it to configure the language server (see below).

A class may reference another class that is defined in a later file, so the files must be loaded in the order given by `luau-lsp.json`. The generator places each forward reference behind an empty `declare class X end` stub that always precedes the full definition.

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

In `.vscode/settings.json`, set `luau-lsp.types.definitionFiles` to the object in `types/luau-lsp.json`, and set `luau-lsp.ignoreGlobs` to `["**/*.d.luau"]`.

```json
{
    "luau-lsp.platform.type": "standard",
    "luau-lsp.types.definitionFiles": {
        "...paste the object from types/luau-lsp.json here, in order...": "..."
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
