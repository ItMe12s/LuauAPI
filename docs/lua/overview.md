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
- Read mod metadata, paths, saves, and settings with `geode.Mod`. See [Reference: mod](reference/mod.md).
- Draw a debug overlay with `imgui`. See [Reference: imgui](reference/imgui.md).
- Share an API with other mods through the global table. See [Sharing APIs between mods](guide/sharing-apis-between-mods.md).

## Globals at a glance

- `print(...)` writes a tab separated line to the log.
- `loadstring(source)` compiles a source string. See [Globals](reference/globals.md).
- `require(path)` loads a sibling module. See [Modules and require](guide/modules-and-require.md).
- `task` schedules callbacks. See [Tasks and time](guide/tasks-and-time.md).
- `time` reads the steady clock and the unix clock.
- `geode` is the game bindings root. It holds `cocos2d`, `gd`, `Mod`, `hook`, `skip`, and `fields`.
- `imgui` draws a Dear ImGui debug overlay. See [Reference: imgui](reference/imgui.md).
- `_G` is the shared global table. Use it to share an API with other mods. See [Sharing APIs between mods](guide/sharing-apis-between-mods.md).

## The geode namespace

`geode.cocos2d` holds engine classes such as `CCDirector`, `CCLabelBMFont`, and `CCSprite`.
`geode.gd` holds Geometry Dash classes such as `GameManager` and `MenuLayer`.
`geode.Mod` holds the host mod id, paths, saves, and settings. See [Reference: mod](reference/mod.md).

These classes are generated from the game bindings, and there are many of them.
Rather than learning them from a list here, you rely on editor autocomplete backed by the generated type stubs.
See [Type stubs and editor setup](reference/type-stubs.md).

## Rules to remember

- Your script runs on the main thread.
- Your script has a time budget, so avoid blocking and infinite loops.
- Errors are caught and written to the log. They do not crash the game.
- When you create a `CCNode`, call `:setID()` with your mod id as a prefix. See [Using game objects](guide/using-game-objects.md).

## Tools

The mod ships a built-in script executor. It is an ImGui window with a code box and an Execute button.
It loads by default and you can turn it off with the `enable-executor` mod setting.

See [Core concepts](../getting-started/concepts.md) for the full model.

## Source

- `src/lua/bindings/task/TaskBinding.cpp`
- `tools/luau_codegen/emit/luau_types/`
- `src/lua/module/Requirer.cpp`
- `src/lua/runtime/Runtime.cpp`
