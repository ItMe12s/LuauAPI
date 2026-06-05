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
- Build interface with the Geode UI factories on `geode` (layouts, buttons, popups). See [UI and layouts](guide/ui-and-layouts.md).
- Store per object data with `geode.fields`.
- Read mod metadata, paths, saves, and settings with `geode.Mod`. See [Reference: mod](reference/mod.md).
- Encode/decode JSON with `geode.json`, and read/write mod files with `geode.fs`. See [Reference: json](reference/json.md) and [Reference: fs](reference/fs.md).
- Pass Luau functions as C++ callbacks, selector handlers, and delegate tables where bindings support it. See [Reference: callbacks](reference/callbacks.md) and [Reference: delegates](reference/delegates.md).
- Draw a debug overlay with `imgui`. See [Reference: imgui](reference/imgui.md).
- Share an API with other mods through the global table. See [Sharing APIs between mods](guide/sharing-apis-between-mods.md).

## Globals at a glance

- `print(...)` writes a tab separated line to the log.
- `loadstring(source)` compiles a source string. See [Globals](reference/globals.md).
- `require(path)` loads a sibling module. See [Modules and require](guide/modules-and-require.md).
- `task` schedules callbacks. See [Tasks and time](guide/tasks-and-time.md).
- `time` reads the steady clock and the unix clock.
- `geode` is the game bindings root. It holds `cocos2d`, `gd`, `Mod`, `json`, `fs`, `hook`, `skip`, and `fields`.
- `imgui` draws a Dear ImGui debug overlay. See [Reference: imgui](reference/imgui.md).
- `_G` is the shared global table. Use it to share an API with other mods. See [Sharing APIs between mods](guide/sharing-apis-between-mods.md).

## The geode namespace

`geode.cocos2d` holds engine classes such as `CCDirector`, `CCLabelBMFont`, and `CCSprite`.
`geode.gd` holds Geometry Dash classes such as `GameManager` and `MenuLayer`.
`geode.Mod` holds the host mod id, paths, saves, and settings. See [Reference: mod](reference/mod.md).

These classes are generated from the game bindings, and there are many of them.
You do not learn them from a list here. You rely on editor autocomplete backed by the generated type stubs.
See [Type stubs and editor setup](reference/type-stubs.md).

## Reference page layouts

Lua reference pages use one of three layouts. Pick the page that matches what you need.

- **Per-symbol headings** (`## print`, `## geode.json.parse`): small APIs with a few exports. Used by [globals](reference/globals.md), [mod](reference/mod.md), [fs](reference/fs.md), [json](reference/json.md), and [hooks](reference/hooks.md).
- **Types and Functions** (`## Types`, `## Functions`, `### name`): larger APIs with shared types. Used by [task](reference/task.md), [time](reference/time.md), and [imgui](reference/imgui.md).
- **Topic sections** (behavior grouped by concept): pattern APIs without a flat symbol list. Used by [callbacks](reference/callbacks.md) and [delegates](reference/delegates.md).

Guides teach usage. Reference pages list signatures, types, and limits.

## Rules to remember

- Your script runs on the main thread.
- Your script has a time budget, so avoid blocking and infinite loops.
- Errors are caught and written to the log. They do not crash the game.
- When you create a `CCNode`, call `:setID()` with your mod id as a prefix. See [Using game objects](guide/using-game-objects.md).

## Developer mode

The mod ships built-in developer tools, such as a script executor and other experimental utilities.
These tools are off by default. Turn them on with developer mode in the mod settings.

See [Core concepts](../getting-started/chapter-5.md) for the full model.

## Source

- `src/lua/bindings/task/TaskBinding.cpp`
- `tools/luau_codegen/emit/luau_types/`
- `src/lua/module/Requirer.cpp`
- `src/lua/runtime/Runtime.cpp`
