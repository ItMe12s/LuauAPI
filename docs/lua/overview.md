# Lua overview

## Summary

This page lists what a Luau script can use. It is the starting point for script authors.

## What a script can do

A script runs inside the game on the main thread. From a script you can:

- Log with `print`.
- Schedule work with `task` and read clocks with `time`.
- Load other modules with `require`.
- Hook and change game functions with `geode.hook`.
- Read and create game objects through `geode.cocos2d` and `geode.gd`.
- Store per object data with `geode.fields`.
- Share an API with other mods through the global table. See [Sharing APIs between mods](guide/sharing-apis-between-mods.md).

## Globals at a glance

- `print(...)` writes a tab separated line to the log.
- `require(path)` loads a sibling module. See [Modules and require](guide/modules-and-require.md).
- `task` schedules callbacks. See [Tasks and time](guide/tasks-and-time.md).
- `time` reads the steady clock and the unix clock.
- `geode` is the game bindings root. It holds `cocos2d`, `gd`, `hook`, `skip`, and `fields`.
- `_G` is the shared global table. Use it to share an API with other mods. See [Sharing APIs between mods](guide/sharing-apis-between-mods.md).

## The geode namespace

`geode.cocos2d` holds engine classes such as `CCDirector`, `CCLabelBMFont`, and `CCSprite`.
`geode.gd` holds Geometry Dash classes such as `GameManager` and `MenuLayer`.

These classes are generated from the game bindings, and there are many of them.
Rather than learning them from a list here, you rely on editor autocomplete backed by the generated type stubs.
See [Type stubs and editor setup](reference/type-stubs.md).

## Rules to remember

- Your script runs on the main thread.
- Your script has a time budget, so avoid blocking and infinite loops.
- Errors are caught and written to the log. They do not crash the game.

See [Core concepts](../getting-started/concepts.md) for the full model.

## Source

- `src/lua/bindings/task/TaskBinding.cpp`
- `tools/luau_codegen/emit_luau_types.py`
- `src/lua/module/Requirer.cpp`
- `src/lua/runtime/Runtime.cpp`
