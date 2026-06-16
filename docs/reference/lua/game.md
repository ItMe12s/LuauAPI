# game

## Summary

`geode.utils.game` controls the game process itself.
These calls affect every mod and the whole Geometry Dash session, not just your script.

Use them only when the player clearly expects it, such as a restart after a settings change.
See [LuauAPI mod guidelines](../../mod_guidelines.md) before shipping a mod that calls these.

## Warnings

- `exit` and `restart` can close or reload the whole game.
- `launchLoaderUninstaller` starts the loader uninstall flow and can delete save data.
- Index reviewers may reject mods that call these without good reason.
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
Upstream Geode also has a two-argument `restart` with a safe-mode flag on some Windows builds,
but LuauAPI omits it for cross-platform parity.

## launchLoaderUninstaller

```lua
geode.utils.game.launchLoaderUninstaller(deleteSaveData: boolean) -> ()
```

Opens the loader uninstaller on Windows. When `deleteSaveData` is true, saved game data is deleted as part of uninstall.

On iOS, Android, and macOS this call is unsupported and has no effect.

## Related

- [Getting started](../../getting-started/overview.md)
- [LuauAPI mod guidelines](../../mod_guidelines.md)
- [geode.utils](utils.md)
- [globals](globals.md)
- [Limits and errors](../cpp/limits-and-errors.md)

## Source

- `tools/luau_codegen/model/free_fn_sources.py`
- Generated free-function bindings at build time
