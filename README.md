<h1 align="center">LuauAPI</h1>

<p align="center">
  <img src="logo.png" alt="Logo" width="200">
</p>

<p align="center">
  A shared Luau runtime for Geode mods<br>
  with many awesome features and tools.
</p>

<p align="center">
  The documentation is organized by audience.<br>
  Choose the section that matches how you plan to use,<br>
  extend, or contribute to the project.
</p>

**Heads up: LuauAPI is still in beta, so expect things to change.**

## Before you start

You are expected to already have:

- Basic Geode modding knowledge. [Learn Geode SDK here](https://docs.geode-sdk.org).
- A working knowledge of Lua or Luau scripting.
- Experience using VS Code.

*Optionally, you can learn these along the way.*

## Getting started

For mod authors building a Geode mod using Luau scripting.
Runnable samples: [Examples](docs/getting-started/examples.md).

- [Overview](docs/getting-started/overview.md)
- [Installation](docs/getting-started/installation.md)
- [Editor setup](docs/getting-started/editor-setup.md)
- [Your first script](docs/getting-started/first-script.md)
- [Examples](docs/getting-started/examples.md)

## Reference

API reference for everything a script or host can call.

- Lua
  - [base64](docs/reference/lua/base64.md)
  - [Callbacks](docs/reference/lua/callbacks.md)
  - [cocos](docs/reference/lua/cocos.md)
  - [ColorProvider](docs/reference/lua/color-provider.md)
  - [Delegates](docs/reference/lua/delegates.md)
  - [fs](docs/reference/lua/fs.md)
  - [Game objects](docs/reference/lua/game-objects.md)
  - [gd3d](docs/reference/lua/gd3d.md)
  - [Globals](docs/reference/lua/globals.md)
  - [Hooks](docs/reference/lua/hooks.md)
  - [imgui](docs/reference/lua/imgui.md)
  - [json](docs/reference/lua/json.md)
  - [Keybind](docs/reference/lua/keybind.md)
  - [mod](docs/reference/lua/mod.md)
  - [Modules](docs/reference/lua/modules.md)
  - [permission](docs/reference/lua/permission.md)
  - [Sharing APIs between mods](docs/reference/lua/sharing-apis.md)
  - [Tasks and time](docs/reference/lua/tasks.md)
  - [Type stubs](docs/reference/lua/type-stubs.md)
  - [UI and layouts](docs/reference/lua/ui.md)
  - [VersionInfo](docs/reference/lua/version-info.md)
  - [web](docs/reference/lua/web.md)
  - [websocket](docs/reference/lua/websocket.md)
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
  - [Bindings framework](docs/contributor/internals/bindings-framework.md)
  - [ImGui draw scheduler](docs/contributor/internals/imgui-draw-scheduler.md)
  - [Module system](docs/contributor/internals/module-system.md)
  - [Runtime](docs/contributor/internals/runtime.md)
  - [Task scheduler](docs/contributor/internals/task-scheduler.md)
- Codegen
  - [ccCArray fields](docs/contributor/codegen/cc-c-array.md)
  - [Codegen](docs/contributor/codegen/codegen.md)
  - [Nested containers](docs/contributor/codegen/nested-containers.md)
  - [Pair containers](docs/contributor/codegen/pair-containers.md)
  - [Platform parity](docs/contributor/codegen/platform-parity.md)

## Project

- [About](about.md)
- [Changelog](changelog.md)
- [Support](support.md)

## Licenses

- [Catch2](https://github.com/catchorg/Catch2/blob/devel/LICENSE.txt)
- [cgltf](https://github.com/jkuhlmann/cgltf/blob/master/LICENSE)
- [Dear ImGui](https://github.com/ocornut/imgui/blob/master/LICENSE.txt)
- [{fmt}](https://github.com/fmtlib/fmt/blob/main/LICENSE)
- [gd-imgui-cocos](https://github.com/matcool/gd-imgui-cocos/blob/geode/LICENSE)
- [Geode SDK](https://github.com/geode-sdk/geode/blob/main/LICENSE.txt)
- [GLM](https://github.com/g-truc/glm/blob/master/copying.txt)
- [Isocline](https://github.com/daanx/isocline/blob/main/LICENSE)
- [IXWebSocket](https://github.com/machinezone/IXWebSocket/blob/master/LICENSE.txt)
- [Luau](https://github.com/luau-lang/luau/blob/master/LICENSE.txt)
- [stb](https://github.com/nothings/stb/blob/master/LICENSE)
