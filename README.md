<h1 align="center">LuauAPI</h1>

<p align="center">
  <img src="logo.png" alt="Logo" width="200">
</p>

<p align="center">
  A shared Luau runtime for Geode mods.<br>
  LuauAPI lets mods run Luau scripts and call into Geometry Dash through generated bindings.<br>
</p>

<p align="center">
  The documentation is organized by audience.<br>
  Choose the section that matches how you plan to use,<br>
  extend, or contribute to the project.
</p>

**Heads up: LuauAPI is still early, so expect things to change.**

**You are assumed to have basic modding knowledge. [Learn Geode SDK here](https://docs.geode-sdk.org).**

## Getting started

Start here regardless of your role. Read the chapters in order.

- [Chapter 1: Installation](docs/getting-started/chapter-1.md)
- [Chapter 2: Building](docs/getting-started/chapter-2.md)
- [Chapter 3: Editor setup](docs/getting-started/chapter-3.md)
- [Chapter 4: Your first script](docs/getting-started/chapter-4.md)
- [Chapter 5: Core concepts](docs/getting-started/chapter-5.md)

## Lua script authors

For anyone writing `.luau` files that run inside the game.

- [Overview](docs/lua/overview.md)
- Guides
  - [Writing scripts](docs/lua/guide/writing-scripts.md)
  - [Hooks](docs/lua/guide/hooks.md)
  - [Tasks and time](docs/lua/guide/tasks-and-time.md)
  - [Using game objects](docs/lua/guide/using-game-objects.md)
  - [UI and layouts](docs/lua/guide/ui-and-layouts.md)
  - [Modules and require](docs/lua/guide/modules-and-require.md)
  - [Sharing APIs between mods](docs/lua/guide/sharing-apis-between-mods.md)
- Reference
  - [Globals](docs/lua/reference/globals.md)
  - [Hooks](docs/lua/reference/hooks.md)
  - [mod](docs/lua/reference/mod.md)
  - [task](docs/lua/reference/task.md)
  - [time](docs/lua/reference/time.md)
  - [imgui](docs/lua/reference/imgui.md)
  - [json](docs/lua/reference/json.md)
  - [fs](docs/lua/reference/fs.md)
  - [web](docs/lua/reference/web.md)
  - [cocos](docs/lua/reference/cocos.md)
  - [base64](docs/lua/reference/base64.md)
  - [permission](docs/lua/reference/permission.md)
  - [ColorProvider](docs/lua/reference/color-provider.md)
  - [VersionInfo](docs/lua/reference/version-info.md)
  - [Keybind](docs/lua/reference/keybind.md)
  - [Callbacks](docs/lua/reference/callbacks.md)
  - [Delegates](docs/lua/reference/delegates.md)
  - [Type stubs and editor setup](docs/lua/reference/type-stubs.md)

## C++ host integrators

For anyone embedding the runtime in their own Geode mod and running scripts from C++.

- [Integration guide](docs/cpp/integration-guide.md)
- [API reference](docs/cpp/api-reference.md)
- [Limits and errors](docs/cpp/limits-and-errors.md)

## Contributors

For anyone working on the runtime itself.

- [Architecture](docs/dev/architecture.md)
- [Runtime](docs/dev/runtime.md)
- [Bindings framework](docs/dev/bindings-framework.md)
- [Module system](docs/dev/module-system.md)
- [Task scheduler](docs/dev/task-scheduler.md)
- [Codegen](docs/dev/codegen.md)
- [Testing](docs/dev/testing.md)
- [ImGui draw scheduler](docs/dev/imgui-draw-scheduler.md)
- [Platform parity](docs/dev/platform-parity.md)
- [ccCArray fields](docs/dev/cc-c-array.md)
- [Nested containers](docs/dev/nested-containers.md)
- [Pair containers](docs/dev/pair-containers.md)

## Project

- [About](about.md)
- [Support](support.md)
- [Changelog](changelog.md)

## Licenses

- [Luau](https://github.com/luau-lang/luau/blob/master/LICENSE.txt)
- [Geode SDK](https://github.com/geode-sdk/geode/blob/main/LICENSE.txt)
- [gd-imgui-cocos](https://github.com/matcool/gd-imgui-cocos/blob/geode/LICENSE)
- [Dear ImGui](https://github.com/ocornut/imgui/blob/master/LICENSE.txt)
- [Catch2](https://github.com/catchorg/Catch2/blob/devel/LICENSE.txt)
- [{fmt}](https://github.com/fmtlib/fmt/blob/main/LICENSE)
- [Isocline](https://github.com/daanx/isocline/blob/main/LICENSE)
