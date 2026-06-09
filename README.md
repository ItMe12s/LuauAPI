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
**And some knowledge on Lua/Luau scripting!!!**

## Getting started

For mod authors building a Geode mod using Luau scripting.

- [Overview](docs/getting-started/overview.md)
- [Installation](docs/getting-started/installation.md)
- [Your first script](docs/getting-started/first-script.md)
- [Editor setup](docs/getting-started/editor-setup.md)
- [Examples](docs/getting-started/examples.md)

## Reference

API reference for everything a script or host can call.

- Lua
  - [Globals](docs/reference/lua/globals.md)
  - [Hooks](docs/reference/lua/hooks.md)
  - [Tasks and time](docs/reference/lua/tasks.md)
  - [Modules](docs/reference/lua/modules.md)
  - [Game objects](docs/reference/lua/game-objects.md)
  - [UI and layouts](docs/reference/lua/ui.md)
  - [Sharing APIs between mods](docs/reference/lua/sharing-apis.md)
  - [Callbacks](docs/reference/lua/callbacks.md)
  - [Delegates](docs/reference/lua/delegates.md)
  - [mod](docs/reference/lua/mod.md)
  - [fs](docs/reference/lua/fs.md)
  - [json](docs/reference/lua/json.md)
  - [web](docs/reference/lua/web.md)
  - [imgui](docs/reference/lua/imgui.md)
  - [cocos](docs/reference/lua/cocos.md)
  - [base64](docs/reference/lua/base64.md)
  - [permission](docs/reference/lua/permission.md)
  - [ColorProvider](docs/reference/lua/color-provider.md)
  - [VersionInfo](docs/reference/lua/version-info.md)
  - [Keybind](docs/reference/lua/keybind.md)
  - [Type stubs](docs/reference/lua/type-stubs.md)
- C++
  - [API reference](docs/reference/cpp/api-reference.md)
  - [Integration guide](docs/reference/cpp/integration-guide.md)
  - [Limits and errors](docs/reference/cpp/limits-and-errors.md)

## Contributor

For anyone working on LuauAPI itself.

- [Architecture](docs/contributor/architecture.md)
- [Building from source](docs/contributor/building.md)
- [Testing](docs/contributor/testing.md)
- Internals
  - [Runtime](docs/contributor/internals/runtime.md)
  - [Bindings framework](docs/contributor/internals/bindings-framework.md)
  - [Module system](docs/contributor/internals/module-system.md)
  - [Task scheduler](docs/contributor/internals/task-scheduler.md)
  - [ImGui draw scheduler](docs/contributor/internals/imgui-draw-scheduler.md)
- Codegen
  - [Codegen](docs/contributor/codegen/codegen.md)
  - [Platform parity](docs/contributor/codegen/platform-parity.md)
  - [ccCArray fields](docs/contributor/codegen/cc-c-array.md)
  - [Nested containers](docs/contributor/codegen/nested-containers.md)
  - [Pair containers](docs/contributor/codegen/pair-containers.md)

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
