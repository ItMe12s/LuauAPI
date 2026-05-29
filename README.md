# LuauAPI

LuauAPI is a shared Luau runtime for Geode mods. It runs Luau scripts inside Geometry Dash and gives those scripts safe access to the game through generated bindings.

The documentation is organized by audience. Use the section that matches how you work with the project.

## Getting started

Start here regardless of your role.

- [Installation](docs/getting-started/installation.md)
- [Building](docs/getting-started/building.md)
- [Your first script](docs/getting-started/first-script.md)
- [Core concepts](docs/getting-started/concepts.md)

## Lua script authors

For anyone writing `.luau` files that run inside the game.

- [Overview](docs/lua/overview.md)
- Guides
  - [Writing scripts](docs/lua/guide/writing-scripts.md)
  - [Hooks and modify](docs/lua/guide/hooks-and-modify.md)
  - [Tasks and time](docs/lua/guide/tasks-and-time.md)
  - [Using game objects](docs/lua/guide/using-game-objects.md)
  - [Modules and require](docs/lua/guide/modules-and-require.md)
- Reference
  - [Globals](docs/lua/reference/globals.md)
  - [Hooks](docs/lua/reference/hooks.md)
  - [task](docs/lua/reference/task.md)
  - [time](docs/lua/reference/time.md)
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
