# Installation

## Summary

This page lists what you need to build and use LuauAPI. LuauAPI is a Geode mod that ships a shared Luau runtime.

## What LuauAPI is

LuauAPI is published as a Geode mod with the id `imes.luauapi`. It loads early and with first priority, so the runtime is available to other mods as soon as the game starts. It also exports a C++ API that other mods can depend on.

The following facts come from `mod.json`:

- Mod id: `imes.luauapi`
- Geode version: `5.7.1`
- Geometry Dash version: `2.2081` on every platform
- `early-load` is true
- `load-priority` is first
- The C++ API is exported from `include/**/*.hpp`

## Required tools

You need the following tools to build the mod. The versions come from `CMakeLists.txt`.

- CMake 3.21 or newer
- A C++23 compiler
- Python 3.11 or newer, used by the binding code generator
- The Geode SDK, located through the `GEODE_SDK` environment variable

## Fetched dependencies

CMake downloads these for you during configuration, so you do not install them by hand.

- Luau, pinned to tag `0.722`
- Geode bindings, pinned to a fixed commit
- Catch2 `v3.15.0`, fetched only when tests are enabled

## Supported platforms

`CMakeLists.txt` configures code generation for the following targets:

- Windows
- macOS, both arm64 and x86_64
- iOS, arm64
- Android 32 bit and Android 64 bit

## Next steps

- [Building](building.md)
- [Editor setup](editor-setup.md)
- [Your first script](first-script.md)

## Source

- `mod.json`
- `CMakeLists.txt`
