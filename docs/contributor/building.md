# Building from source

## Summary

How to build LuauAPI from this repository.
Build from source only if you want unreleased features or to work on the runtime itself.
Mod authors who just use LuauAPI install it from the Geode index in-game or GitHub releases instead.
See [Installation](../getting-started/installation.md).

## Required tools

Versions come from `CMakeLists.txt`.

- CMake 3.21 or newer
- A compiler with C++23 support
- Python 3.11 or newer, used by the binding code generator
- The Geode SDK, found through the `GEODE_SDK` environment variable

## Fetched dependencies

CMake downloads these during configuration, so you do not install them by hand.

- Luau, pinned to tag `0.726`
- Geode bindings, pinned through `LUAUAPI_BINDINGS_GIT_TAG` in `cmake/GeodeBindings.cmake`
- Dear ImGui `v1.92.8`, pinned in `cmake/ImGui.cmake` for `imgui-cocos` and headless tests
- GLM `1.0.3`, used by the 3D math and glTF loader
- IXWebSocket `v12.0.0` and mbedTLS `v3.6.6`, fetched for websocket support
- Catch2 `v3.15.0`, fetched only when tests are on

## Vendored dependencies

These live in the repository.

- `gd-imgui-cocos/` is a LuauAPI fork of [matcool/gd-imgui-cocos](https://github.com/matcool/gd-imgui-cocos).
  It is not a standalone library. LuauAPI wires it through `cmake/ImGuiCocos.cmake`.
  See [gd-imgui-cocos](../../gd-imgui-cocos/README.md).

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

- `geode build` runs the Geode CLI, which configures and builds the mod.
  Auto-install needs a configured Geode CLI profile. Without one, the package stays in the build folder.
- In VSCode, run `CMake: Configure` then `CMake: Build` from the Command Palette.

Python 3.11+ must be on PATH before you configure, because the first configure runs the codegen listing.
For platform builds, use the Geode CLI or the CI workflow in `.github/workflows/multi-platform.yml`.

## Code generation

The build runs the `luauapi_codegen` target before the main library.
See [Codegen](codegen/codegen.md).

## Supported platforms

See [Installation](../getting-started/installation.md) for the supported platform list.
Other Android ABIs fail at configure time.
`LUAUAPI_HOST_ONLY=ON` skips the Geode SDK and configure-time codegen listing for host-only tooling.

## Tests

Tests are off by default. Turn them on with `-DLUAUAPI_BUILD_TESTS=ON`, build, then run CTest.
See [Testing](testing.md).

## Format check

Before pushing changes, run the format script from the repo root.
You need `clang-format` and `ruff` on PATH.

```powershell
scripts/ci/format.ps1 -Check
```

The script formats C++ under `src/`, `include/`, and `tests/`, but skips `tests/host/`.
It runs `clang-format` once per file.
It runs `ruff format` on `tools/` and `tests/luau_codegen/`.

Pass `-Target clang`, `-Target python`, or omit `-Target` for both.
Drop `-Check` to rewrite files in place.
The script prints a short summary when each step finishes.

## CI

See [Testing](testing.md) for CI jobs, how to run tests, and the host test list.

## Related

- [Installation](../getting-started/installation.md)
- [Codegen](codegen/codegen.md)
- [Testing](testing.md)

## Source

- `CMakeLists.txt`
- `cmake/ImGui.cmake`
- `cmake/ImGuiCocos.cmake`
- `mod.json`
- `scripts/ci/format.ps1`
- `tools/luau_codegen/cli/main.py`
