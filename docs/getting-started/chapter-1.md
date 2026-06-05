# Chapter 1: Installation

## Summary

This page lists what you need to build and use LuauAPI. It is a Geode mod that ships a shared Luau runtime.

## What LuauAPI is

LuauAPI ships as a Geode mod with the id `imes.luauapi`.
It loads early and with first priority, so the runtime is ready for other mods as soon as the game starts.
It also exports a C++ API that other mods can depend on.

From `mod.json`:

- Mod id: `imes.luauapi`
- Geode version: `5.7.1`
- Geometry Dash version: `2.2081` on every platform
- `early-load` is true
- `load-priority` is first
- The C++ API is exported from `include/**/*.hpp`

## Settings

- `enable-executor`: turns on the built-in ImGui script executor (on by default).

## Required tools

You need these tools to build the mod. The versions come from `CMakeLists.txt`.

- CMake 3.21 or newer
- A C++ compiler with C++26 support. The build sets `CMAKE_CXX_STANDARD` to 26 and needs at least the C++23 feature set.
- Python 3.11 or newer, used by the binding code generator
- The Geode SDK, found through the `GEODE_SDK` environment variable

## Fetched dependencies

CMake downloads these during configuration, so you do not install them by hand.

- Luau, pinned to tag `0.723`
- Geode bindings, pinned to commit `00328ccd2fd3e4b005a54eaaa4d4d91e22ca7df4` (`LUAUAPI_BINDINGS_GIT_TAG` in `CMakeLists.txt`)
- gd-imgui-cocos, pinned to commit `b93f08ccef778a53ebba09b20c347f6a63980119` (`LUAUAPI_IMGUI_COCOS_GIT_TAG` in `CMakeLists.txt`). It also pins Dear ImGui through its own CPM setup. Only the LuauAPI mod links it. Other mods use the `imgui` Lua API instead of adding their own dependency.
- Catch2 `v3.15.0`, fetched only when tests are on

To bump bindings for a new Geometry Dash version,
update `mod.json` and the `LUAUAPI_BINDINGS_GIT_TAG` cache variable to a matching commit from the Geode bindings repo.

LuauAPI sets `GEODE_BINDINGS_REPO_PATH` to that same pinned fetch for the Geode SDK configure step,
so Geode Codegen does not clone bindings `main` on its own.
Override the `GEODE_BINDINGS_REPO_PATH` environment variable only if you mean to use another bindings checkout.

## Supported platforms

`CMakeLists.txt` sets up code generation for these targets:

- Windows
- macOS, both arm64 and x86_64
- iOS, arm64
- Android 32-bit and Android 64-bit

Other Android ABIs fail at configure time. See the platform mapping in [Codegen](../dev/codegen.md).

## Next

- [Chapter 2: Building](chapter-2.md)

## Source

- `mod.json`
- `CMakeLists.txt`
