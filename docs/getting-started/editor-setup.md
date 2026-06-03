# Editor setup

## Summary

This guide sets up VSCode so your `.luau` files get autocomplete and type checks for `geode`, `cocos2d`, `gd`, `task`, and `time`.
You build LuauAPI to generate the type stubs, copy them into your mod, then point the Luau language server at them.
Stub file layout and overload policy: [Type stubs](../lua/reference/type-stubs.md).

## Step 1: install the extension

Install VSCode and the Luau Language Server extension, `luau-lsp` by JohnnyMorganz.

## Step 2: build LuauAPI to get the types

The type stubs are generated during the build, into the `types/` folder.
They are not committed to the repo, so you must build LuauAPI yourself to produce them.

```bash
cmake -B build
cmake --build build
```

After the build, `types/` holds the `.d.luau` stub file.
See [Building](building.md) for details.

## Step 3: copy the types folder into your mod

Copy the whole `types/` folder from the LuauAPI repo into the root of your own mod repo.

The generator writes a single stub file:

```text
types/geode.d.luau   all bound classes, factories, enum aliases, geode namespace, task, time, loadstring, and imgui
```

Extra binding stubs from `tools/luau_codegen/extra_bindings/` are merged into this file at build time.

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

`luau-lsp.types.definitionFiles` maps a name to the stub path. There is a single file, so the object has one entry.

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

The bindings stub is large.
The fflags override prevents typecheck errors about code complexity.
Keep these values in place.

## Step 6: ignore the types folder in git

The stubs are generated output, not source. Add this line to your mod's `.gitignore` so you do not commit them.

```text
/types
```

## Step 7: reload

Reload the VSCode window so the language server reads the new config. Open a `.luau` file and type `geode.` to confirm autocomplete works.

## If you develop LuauAPI in this repo

After pulling changes that affect the bindings:

1. Rebuild (`cmake --build build`) to regenerate `types/geode.d.luau`.
2. Reload the window. The `definitionFiles` entry is a fixed single path, so there is nothing to re-paste.

## Keeping types current

When LuauAPI updates its bindings, rebuild it and recopy the `types/` folder into your mod.

## Related

- [Building](building.md)
- [Type stubs and editor setup](../lua/reference/type-stubs.md)
- [Your first script](first-script.md)

## Source

- `.luaurc` (shipped in the LuauAPI repo)
- `.vscode/settings.json` (create in your mod, see Step 5)
- `tools/luau_codegen/emit/luau_types/`
- `docs/getting-started/building.md`
- `CMakeLists.txt`
