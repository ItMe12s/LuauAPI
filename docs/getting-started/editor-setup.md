# Editor setup

## Summary

This guide sets up VSCode so your `.luau` files get autocomplete and type checks for `geode`, `cocos2d`, `gd`, `task`, and `time`. You build LuauAPI to generate the type stubs, copy them into your mod, then point the Luau language server at them.

## Step 1: install the extension

Install VSCode and the Luau Language Server extension, `luau-lsp` by JohnnyMorganz.

## Step 2: build LuauAPI to get the types

The type stubs are generated during the build, into the `types/` folder. They are not committed to the repo, so you must build LuauAPI yourself to produce them.

```bash
cmake -B build
cmake --build build
```

After the build, `types/` holds the `.d.luau` stub files. See [Building](building.md) for details.

## Step 3: copy the types folder into your mod

Copy the whole `types/` folder from the LuauAPI repo into the root of your own mod repo.

Your mod root should now contain a `types/` folder with these files:

```text
types/geode_cocos2d_all.d.luau
types/geode_gd_all.d.luau
types/geode.d.luau
```

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

Create `.vscode/settings.json` in your mod root.

Keep the `definitionFiles` order exactly: cocos bundle, then gd bundle, then the geode root.

```json
{
    "luau-lsp.platform.type": "standard",
    "luau-lsp.types.definitionFiles": {
        "@geode-cocos2d-all": "types/geode_cocos2d_all.d.luau",
        "@geode-gd-all": "types/geode_gd_all.d.luau",
        "@geode": "types/geode.d.luau"
    },
    "luau-lsp.ignoreGlobs": [
        "**/*.d.luau"
    ]
}
```

## Step 6: ignore the types folder in git

The stubs are generated output, not source. Add this line to your mod's `.gitignore` so you do not commit them.

```text
/types
```

## Step 7: reload

Reload the VSCode window so the language server reads the new config. Open a `.luau` file and type `geode.` to confirm autocomplete works.

## Keeping types current

When LuauAPI updates its bindings, rebuild it and recopy the `types/` folder into your mod. The stubs are platform specific, so build on the platform you develop on.

## Related

- [Building](building.md)
- [Type stubs and editor setup](../lua/reference/type-stubs.md)
- [Your first script](first-script.md)

## Source

- `.luaurc`
- `.vscode/settings.json`
- `tools/luau_codegen/emit_luau_types.py`
- `docs/getting-started/building.md`
- `CMakeLists.txt`
