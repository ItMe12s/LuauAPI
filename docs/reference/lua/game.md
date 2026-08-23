# game

## Summary

`geode.utils.game` controls the game process itself.
These calls affect every mod and the whole Geometry Dash session, not just your script.

Use them only when the player clearly expects it, such as a restart after a settings change.
See [LuauAPI mod guidelines](../../mod_guidelines.md) before shipping a mod that calls these.

## Warnings

- `exit` and `restart` can close or reload the whole game.
- `launchLoaderUninstaller` starts the loader uninstall flow and can delete save data.
- Reviewers may reject mods that call these without good reason. Policy lives in [LuauAPI mod guidelines](../../mod_guidelines.md).
- There is no undo. Test on a dev build first.

## exit

```lua
geode.utils.game.exit(saveData: boolean) -> ()
```

Quits the game. When `saveData` is true, Geode saves game data before exiting.

## restart

```lua
geode.utils.game.restart(saveData: boolean) -> ()
```

Restarts the game. When `saveData` is true, Geode saves game data before restarting.

The published Luau stub exposes one boolean only.
Upstream Geode also has a two-argument `restart` with a safe-mode flag.
That overload is missing on some platforms, so LuauAPI binds the one-boolean form everywhere.

## launchLoaderUninstaller

```lua
geode.utils.game.launchLoaderUninstaller(deleteSaveData: boolean) -> ()
```

Opens the loader uninstaller on Windows. When `deleteSaveData` is true, saved game data is deleted as part of uninstall.

On iOS, Android, and macOS this call is unsupported.
Geode logs an error and does not launch the uninstaller.

## Related

- [geode.utils](utils.md)
- [LuauAPI mod guidelines](../../mod_guidelines.md)
- [globals](globals.md)
- [Limits and errors](../cpp/limits-and-errors.md)
- [Getting started](../../getting-started/overview.md)

## Source

- `tools/luau_codegen/model/free_fn_sources.py`
- `tools/luau_codegen/policy/free_functions.py`
- `build/luauapi-gen/src/bindings_free_functions.cpp`
