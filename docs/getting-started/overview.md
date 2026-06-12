# Getting started

## Summary

LuauAPI is a Geode mod that ships a shared Luau runtime.
It lets your mod run Luau scripts and call into Geometry Dash through generated bindings.
This page is the entry point for mod authors who want to build a Geode mod using Luau,
whether a brand new mod or one you already have.

## The two ways to use it

1. Build a Geode mod that depends on LuauAPI and runs your `.luau` files from its C++.
   This is the common path and needs no build steps. Most mods using LuauAPI are built this way.
   See [Installation](installation.md) and [Your first script](first-script.md).
2. Build LuauAPI from source if you want unreleased features or to work on the runtime itself.
   That path lives in the contributor docs. See [Building](../contributor/building.md).

## Key concepts

- One runtime, one main thread. The runtime is created once and reused.
  Almost every entry point must run on the main thread, and the runtime checks this on each call.
- Resources root and flat paths. Scripts load from a resources directory the host passes.
  Script and module names are flat single file names with the `.luau` extension.
  No folders, no `..`, no absolute paths.
- Deadlines and memory. Each run has a time budget in milliseconds. Going over raises an error.
  Memory has a hard cap with no soft limit.
  See [Limits and errors](../reference/cpp/limits-and-errors.md).
- Errors are logged, not fatal. The host runs scripts in a protected call,
  so an error is caught and written to the log instead of crashing the game.

## Next

- [Installation](installation.md)
- [Your first script](first-script.md)
- [Editor setup](editor-setup.md)
- [Examples](examples.md)

## Related

- [Lua reference](../reference/lua/globals.md)
- [C++ integration guide](../reference/cpp/integration-guide.md)

## Source

- `mod.json`
- `src/main.cpp`
- `src/api.cpp`
- `src/core/Runtime.cpp`
- `src/core/Config.hpp`
