<h1 align="center">LuauAPI</h1>

<p align="center">
  <img src="logo.png" alt="Logo" width="200">
</p>

<p align="center">
  LuauAPI is a shared Luau runtime for Geode mods.<br>
  It lets Luau scripts safely access Geometry Dash through generated bindings,<br>
  so you can write full mods using only Luau.
</p>

<p align="center">
  The documentation is organized by audience.<br>
  Choose the section that matches how you plan to use,<br>
  extend, or contribute to the project.
</p>

**Early and in active development, APIs may change.**

## Getting started

Start here regardless of your role.

- [Installation](docs/getting-started/installation.md)
- [Building](docs/getting-started/building.md)
- [Editor setup](docs/getting-started/editor-setup.md)
- [Your first script](docs/getting-started/first-script.md)
- [Core concepts](docs/getting-started/concepts.md)

## Lua script authors

For anyone writing `.luau` files that run inside the game.

- [Overview](docs/lua/overview.md)
- Guides
  - [Writing scripts](docs/lua/guide/writing-scripts.md)
  - [Hooks](docs/lua/guide/hooks.md)
  - [Tasks and time](docs/lua/guide/tasks-and-time.md)
  - [Using game objects](docs/lua/guide/using-game-objects.md)
  - [Modules and require](docs/lua/guide/modules-and-require.md)
  - [Sharing APIs between mods](docs/lua/guide/sharing-apis-between-mods.md)
- Reference
  - [Globals](docs/lua/reference/globals.md)
  - [Hooks](docs/lua/reference/hooks.md)
  - [mod](docs/lua/reference/mod.md)
  - [task](docs/lua/reference/task.md)
  - [time](docs/lua/reference/time.md)
  - [imgui](docs/lua/reference/imgui.md)
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
