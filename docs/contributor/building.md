# Building from source

## Summary

How to build LuauAPI from this repository.
Build from source only if you want unreleased features or to work on the runtime itself.
Mod authors who just use LuauAPI install it from the Geode index instead.
See [Installation](../getting-started/installation.md).

## Required tools

Versions come from `CMakeLists.txt`.

- CMake 3.21 or newer
- A compiler with C++23 support
- Python 3.11 or newer, used by the binding code generator
- The Geode SDK, found through the `GEODE_SDK` environment variable

## Fetched dependencies

CMake downloads these during configuration, so you do not install them by hand.

- Luau, pinned to tag `0.725`
- Geode bindings, pinned through `LUAUAPI_BINDINGS_GIT_TAG` in `CMakeLists.txt`
- gd-imgui-cocos, pinned through `LUAUAPI_IMGUI_COCOS_GIT_TAG`.
  It pins Dear ImGui through its own CPM setup. Only the LuauAPI mod links it.
  Other mods use the `imgui` Lua API instead.
- Catch2 `v3.15.0`, fetched only when tests are on
- GLM `1.0.3`, used by the 3D math and glTF loader

Vendored headers in `vendor/` are not fetched by CMake:

- `cgltf.h` for glTF parsing. Define `CGLTF_IMPLEMENTATION` in one translation unit only (`GltfIo.cpp`).
- `stb_image.h` for PNG and JPEG decode. Define `STB_IMAGE_IMPLEMENTATION` in one translation unit only (`ImageDecode.cpp`).

The `gd3d` stack is always built into the mod. There is no CMake option to disable it.
OpenGL comes from Geode and cocos2d, not from a standalone `find_package(OpenGL)` call.

To bump bindings for a new Geometry Dash version, update `mod.json`
and the `LUAUAPI_BINDINGS_GIT_TAG` cache variable to a matching commit from the Geode bindings repo.
LuauAPI sets `GEODE_BINDINGS_REPO_PATH` to that same pinned fetch
so Geode Codegen does not clone bindings `main` on its own.

## Build

```bash
cmake -B build
cmake --build build
```

Two shortcuts do the same thing:

- `geode build` runs the Geode CLI, which configures, builds, and installs the mod.
- In VSCode, run `CMake: Configure` then `CMake: Build` from the Command Palette.

Python 3.11+ must be on PATH before you configure, because the first configure runs the codegen listing.
For platform builds, use the Geode CLI or the CI workflow in `.github/workflows/multi-platform.yml`.

## Code generation

The build runs the `luauapi_codegen` target before the main library.
See [Codegen](codegen/codegen.md).

## Supported platforms

Codegen is set up for Windows, macOS (arm64 and x86_64), iOS (arm64), and Android (32-bit and 64-bit).
Other Android ABIs fail at configure time. `LUAUAPI_HOST_ONLY=ON` skips the Geode SDK
and configure-time codegen listing, for host-only tooling without `GEODE_SDK`.

## Tests

Tests are off by default. Turn them on with `-DLUAUAPI_BUILD_TESTS=ON`, build, then run CTest.
See [Testing](testing.md).

## Format check

Before pushing C++ changes, run the format script from the repo root:

```powershell
scripts/ci/format.ps1 -Check
```

Pass `-Target clang`, `-Target python`, or omit `-Target` for both.
Drop `-Check` to rewrite files in place.

## CI

Continuous integration is defined in `.github/workflows/multi-platform.yml`:

- **Windows Tests** and **macOS Tests** configure with `-DLUAUAPI_BUILD_TESTS=ON`, build `luauapi_tests`, and run CTest.
- **Linux Codegen Tests** run the Python suite under `tests/luau_codegen/`.
- The **build** matrix produces the mod for Windows, macOS, iOS, and Android ABIs.
- **Package builds** combines matrix artifacts for release.

Host tests link IXWebSocket and mbedTLS for websocket coverage.
See [Testing](testing.md) for the full host test list and link dependencies.

## Related

- [Installation](../getting-started/installation.md)
- [Codegen](codegen/codegen.md)
- [Testing](testing.md)

## Source

- `CMakeLists.txt`
- `mod.json`
- `tools/luau_codegen/cli/main.py`
