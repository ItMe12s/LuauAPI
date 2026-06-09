# Building from source

## Summary

How to build LuauAPI from this repository. Build from source only if you want unreleased features or
to work on the runtime itself. Mod authors who just use LuauAPI install it from the Geode index
instead. See [Installation](../getting-started/installation.md).

## Required tools

Versions come from `CMakeLists.txt`.

- CMake 3.21 or newer
- A C++ compiler with C++26 support. The build sets `CMAKE_CXX_STANDARD` to 26 and needs at least
  the C++23 feature set.
- Python 3.11 or newer, used by the binding code generator
- The Geode SDK, found through the `GEODE_SDK` environment variable

## Fetched dependencies

CMake downloads these during configuration, so you do not install them by hand.

- Luau, pinned to tag `0.723`
- Geode bindings, pinned through `LUAUAPI_BINDINGS_GIT_TAG` in `CMakeLists.txt`
- gd-imgui-cocos, pinned through `LUAUAPI_IMGUI_COCOS_GIT_TAG`. It pins Dear ImGui through its own
  CPM setup. Only the LuauAPI mod links it. Other mods use the `imgui` Lua API instead.
- Catch2 `v3.15.0`, fetched only when tests are on

To bump bindings for a new Geometry Dash version, update `mod.json` and the
`LUAUAPI_BINDINGS_GIT_TAG` cache variable to a matching commit from the Geode bindings repo. LuauAPI
sets `GEODE_BINDINGS_REPO_PATH` to that same pinned fetch so Geode Codegen does not clone bindings
`main` on its own.

## Build

```bash
cmake -B build
cmake --build build
```

Two shortcuts do the same thing:

- `geode build` runs the Geode CLI, which configures, builds, and installs the mod.
- In VSCode, run `CMake: Configure` then `CMake: Build` from the Command Palette.

Python 3.11+ must be on PATH before you configure, because the first configure runs the codegen
listing. For platform builds, use the Geode CLI or the CI workflow in
`.github/workflows/multi-platform.yml`.

## Code generation

The build runs the `luauapi_codegen` target before it compiles the main library. It reads Broma
binding files and writes generated C++ plus `types/geode.d.luau`. Do not edit generated files by
hand. Change the generator or its inputs, then rebuild. See [Codegen](codegen/codegen.md).

## Supported platforms

Codegen is set up for Windows, macOS (arm64 and x86_64), iOS (arm64), and Android (32-bit and
64-bit). Other Android ABIs fail at configure time. `LUAUAPI_HOST_ONLY=ON` skips the Geode SDK and
configure-time codegen listing, for host-only tooling without `GEODE_SDK`.

## Tests

Tests are off by default. Turn them on with `-DLUAUAPI_BUILD_TESTS=ON`, build, then run CTest. See
[Testing](testing.md).

## Related

- [Installation](../getting-started/installation.md)
- [Codegen](codegen/codegen.md)
- [Testing](testing.md)

## Source

- `CMakeLists.txt`
- `mod.json`
- `tools/luau_codegen/cli/main.py`
