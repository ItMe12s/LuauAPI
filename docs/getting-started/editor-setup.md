# Editor setup

## Summary

Set up VSCode so your `.luau` files get autocomplete and type checks for `geode`, `cocos2d`, `gd`,
`task`, and `time`. You get the `geode.d.luau` type stub, place it in your mod, then point the Luau
language server at it.

## Step 1: install the extension

Install VSCode and the Luau Language Server extension, `luau-lsp` by JohnnyMorganz.

## Step 2: get the type stub

Download `geode.d.luau` from the LuauAPI GitHub release tab. It sits next to the `.geode` mod file.
No build is needed. If you build LuauAPI from source instead, the stub is written to `types/` after
a build. See [Building from source](../contributor/building.md).

The stub is one file that holds all bound classes, factories, enum aliases, the `geode` namespace,
the `task` and `time` APIs, `loadstring`, and the ImGui APIs.

## Step 3: place the stub

Put `geode.d.luau` in a `types/` folder at your mod root, so the path is `types/geode.d.luau`.

## Step 4: add .luaurc

Create `.luaurc` in your mod root.

```json
{
    "languageMode": "strict",
    "aliases": {
        "types": "types"
    }
}
```

## Step 5: add .vscode/settings.json

Create `.vscode/settings.json` in your mod root. The stub is large, so the fflags override stops
type-check errors about code complexity. Keep these values.

```json
{
    "luau-lsp.platform.type": "standard",
    "luau-lsp.types.definitionFiles": {
        "@geode": "types/geode.d.luau"
    },
    "luau-lsp.ignoreGlobs": [
        "**/*.d.luau"
    ],
    "luau-lsp.fflags.sync": false,
    "luau-lsp.fflags.override": {
        "LuauTarjanChildLimit": "0",
        "LuauTypeInferIterationLimit": "0",
        "LuauNormalizeCacheLimit": "0",
        "LuauTypeInferRecursionLimit": "1000",
        "LuauVisitRecursionLimit": "1000"
    }
}
```

## Step 6: ignore the types folder in git

The stub is generated output, not source. Add this to your mod `.gitignore`.

```text
# LuauAPI types
/types
```

## Step 7: reload

Reload the VSCode window, open a `.luau` file, and type `geode.` to confirm autocomplete works.

## Keeping types current

When LuauAPI updates its bindings, download the new `geode.d.luau` from the release tab and replace
your copy. The `definitionFiles` entry is a fixed single path, so you only reload after a swap.

## Next

- [Examples](examples.md)

## Related

- [Type stubs](../reference/lua/type-stubs.md)
- [Building from source](../contributor/building.md)

## Source

- `.luaurc`
- `tools/luau_codegen/emit/luau_types/`
- `CMakeLists.txt`
