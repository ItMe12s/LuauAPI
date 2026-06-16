# LuauAPI documentation

## Summary

This folder holds all LuauAPI docs. Pages are grouped by audience.
Start with the section that matches your goal.
Read [Documentation style](STYLE.md) before you edit or add pages.

LuauAPI is in beta. Expect API and doc changes.

## Getting started

For mod authors who depend on LuauAPI and run `.luau` scripts from a Geode mod.

Read [LuauAPI mod guidelines](mod_guidelines.md) before you ship.

- [Getting started](getting-started/overview.md)
- [Installation](getting-started/installation.md)
- [Editor setup](getting-started/editor-setup.md)
- [Your first script](getting-started/first-script.md)
- [Examples](getting-started/examples.md)

## Reference

API reference for scripts and C++ hosts.
Start at [globals](reference/lua/globals.md) for the full Lua module index.

### Lua

Core:

- [globals](reference/lua/globals.md)
- [modules](reference/lua/modules.md)
- [sharing APIs between mods](reference/lua/sharing-apis.md)
- [hooks](reference/lua/hooks.md)
- [tasks and time](reference/lua/tasks.md)
- [callbacks](reference/lua/callbacks.md)
- [delegates](reference/lua/delegates.md)
- [type stubs](reference/lua/type-stubs.md)

Game:

- [enums](reference/lua/enums.md)
- [game objects](reference/lua/game-objects.md)
- [cocos](reference/lua/cocos.md)
- [ColorProvider](reference/lua/color-provider.md)
- [Keybind](reference/lua/keybind.md)
- [Keyboard input](reference/lua/keyboard-input.md)

UI:

- [UI and layouts](reference/lua/ui.md)
- [imgui](reference/lua/imgui.md)
- [gd3d](reference/lua/gd3d.md)

IO:

- [mod](reference/lua/mod.md)
- [fs](reference/lua/fs.md)
- [json](reference/lua/json.md)

Network:

- [web](reference/lua/web.md)
- [websocket](reference/lua/websocket.md)

Utils:

- [geode.utils](reference/lua/utils.md)
- [clipboard](reference/lua/clipboard.md)
- [string](reference/lua/string.md)
- [random](reference/lua/random.md)
- [game](reference/lua/game.md)
- [base64](reference/lua/base64.md)
- [permission](reference/lua/permission.md)
- [VersionInfo](reference/lua/version-info.md)

### C++

- [C++ API reference](reference/cpp/api-reference.md)
- [C++ integration guide](reference/cpp/integration-guide.md)
- [Limits and errors](reference/cpp/limits-and-errors.md)

## Contributor

For people who build LuauAPI from source or work on the runtime and codegen.

- [Architecture](contributor/architecture.md)
- [Building from source](contributor/building.md)
- [Testing](contributor/testing.md)

Internals:

- [Bindings framework](contributor/internals/bindings-framework.md)
- [ImGui draw scheduler](contributor/internals/imgui-draw-scheduler.md)
- [Module system](contributor/internals/module-system.md)
- [Runtime](contributor/internals/runtime.md)
- [Task scheduler](contributor/internals/task-scheduler.md)

Codegen:

- [Codegen](contributor/codegen/codegen.md)
- [CCArray methods](contributor/codegen/cc-array.md)
- [ccCArray read-only fields](contributor/codegen/cc-c-array.md)
- [Nested containers](contributor/codegen/nested-containers.md)
- [Pair containers](contributor/codegen/pair-containers.md)
- [Platform parity](contributor/codegen/platform-parity.md)

## Policy and style

- [LuauAPI mod guidelines](mod_guidelines.md)
- [Documentation style](STYLE.md)

## Related

- [Project README](../README.md)
- [Getting started](getting-started/overview.md)
- [globals](reference/lua/globals.md)
- [Codegen](contributor/codegen/codegen.md)

## Source

- `docs/`
