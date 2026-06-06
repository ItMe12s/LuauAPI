# Chapter 3: Editor setup

## Summary

This page sets up VSCode so your `.luau` files get autocomplete and type checks for `geode`, `cocos2d`, `gd`, `task`, and `time`.
You get the `geode.d.luau` type stub, place it in your mod, then point the Luau language server at it.
For the stub file layout and overload policy, see [Type stubs](../lua/reference/type-stubs.md).

## Step 1: install the extension

Install VSCode and the Luau Language Server extension, `luau-lsp` by JohnnyMorganz.

## Step 2: get the type stub

Pick the path that fits you.

### For API users (recommended)

1. Install LuauAPI from the Geode index in-game or LuauAPI GitHub release tab.
2. Download `geode.d.luau` from the LuauAPI GitHub release tab (this file is located next to the `.geode` mod file).

No build is needed for this setup. This follows the standard Geode SDK mod workflow.

### For developers / latest features

Build LuauAPI from the repository to generate the stub yourself.
Run `geode build`, or `cmake -B build` + `cmake --build build`, or use the VSCode Command Palette.
After the build, the `types/` folder holds the `geode.d.luau` stub.
See [Chapter 2: Building](chapter-2.md).

The stub is a single file, `geode.d.luau`, that contains all declared types.

## Step 3: place the stub in your mod

Put `geode.d.luau` in a `types/` folder at the root of your own mod repo,
so the path is `types/geode.d.luau`.
Whether you downloaded it from the release tab or built it, the file is the same.

This file includes:

- All bound classes
- Factory functions
- Enum aliases
- The `geode` namespace
- The `task` and `time` APIs
- `loadstring` support
- ImGui APIs

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

`luau-lsp.types.definitionFiles` maps a name to the stub path. There is one file, so the object has one entry.

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
The fflags override stops typecheck errors about code complexity.
Keep these values in place.

## Step 6: ignore the types folder in git

The stubs are generated output, not source. Add this line to your mod's `.gitignore` so you do not commit them.

```text
# LuauAPI types
/types
```

## Step 7: reload

Reload the VSCode window so the language server reads the new config.
Open a `.luau` file and type `geode.` to confirm autocomplete works.

## If you develop LuauAPI in this repo

After you pull changes that affect the bindings:

1. Rebuild to regenerate `types/geode.d.luau`.
2. Reload the window. The `definitionFiles` entry is a fixed single path, so there is nothing to re-paste.

## Keeping types current

When LuauAPI updates its bindings, refresh your stub:

- **API users:** download the new `geode.d.luau` from the release tab and replace your copy.
- **Developers:** rebuild LuauAPI and recopy the file into your mod.

## Related

- [Type stubs and editor setup](../lua/reference/type-stubs.md)

## Next

- [Chapter 4: Your first script](chapter-4.md)

Back to [Chapter 2: Building](chapter-2.md).

## Source

- `.luaurc` (shipped in the LuauAPI repo)
- `.vscode/settings.json` (create in your mod, see Step 5)
- `tools/luau_codegen/emit/luau_types/`
- `docs/getting-started/chapter-2.md`
- `CMakeLists.txt`
