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

The stubs are generated as bundled `.d.luau` files (about 50 classes per bundle), so `types/` holds a small set of files:

```text
types/geode_cocos2d_NNN.d.luau        bundled cocos2d classes (NNN = 000, 001, ...)
types/geode_gd_NNN.d.luau             bundled Geometry Dash classes
types/geode_cocos2d_factories.d.luau  cocos2d factories + enum aliases
types/geode_gd_factories.d.luau       Geometry Dash factories + enum aliases
types/geode.d.luau                    the geode namespace root
types/luau-lsp.json                   the ordered list of the files above
```

Bundling keeps each file small enough for luau-lsp typechecking while avoiding Windows `spawn ENAMETOOLONG` when the editor starts the language server with hundreds of definition paths. Always copy the entire `types/` folder and use the exact order from `luau-lsp.json`.

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

`luau-lsp.types.definitionFiles` is a name-to-path object whose entries load in order.
The build writes the complete, correctly ordered object to `types/luau-lsp.json`.
Set `definitionFiles` to it: open `types/luau-lsp.json` and paste its contents as the value.

```json
{
    "luau-lsp.platform.type": "standard",
    "luau-lsp.types.definitionFiles": {
        "...paste the full object from types/luau-lsp.json here, in order...": "..."
    },
    "luau-lsp.ignoreGlobs": [
        "**/*.d.luau"
    ]
}
```

The order in `luau-lsp.json` is required:
a class may reference another that is defined later, so the files must load in exactly that sequence.
Do not reorder or drop entries. Re-paste the object whenever you regenerate the stubs.

## Step 6: ignore the types folder in git

The stubs are generated output, not source. Add this line to your mod's `.gitignore` so you do not commit them.

```text
/types
```

## Step 7: reload

Reload the VSCode window so the language server reads the new config. Open a `.luau` file and type `geode.` to confirm autocomplete works.

## If you develop LuauAPI in this repo

After pulling changes that affect type stub layout (for example bundled stubs instead of one file per class):

1. Reconfigure CMake (`cmake -B build`) so the build knows the new generated type filenames.
2. Rebuild (`cmake --build build`) to regenerate `types/`.
3. Open `types/luau-lsp.json` and paste its object into `.vscode/settings.json` under `luau-lsp.types.definitionFiles` (replace the old per-class list).
4. Reload the window and confirm the Luau Language Server output shows no `spawn ENAMETOOLONG`.
5. If luau-lsp reports "Code is too complex to typecheck" on a bundle, lower `CLASSES_PER_BUNDLE` in `tools/luau_codegen/emit_luau_types.py`, rebuild, and re-paste `luau-lsp.json`.

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
