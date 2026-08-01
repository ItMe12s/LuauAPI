<p align="center">
  <img src="bigLogo.png" alt="Logo" width="600">
</p>

<p align="center">
  A shared Luau runtime for Geode mods<br>
  with many awesome features and tools.
</p>

# LuauAPI

**Heads up: LuauAPI is still in beta, so expect things to change.**
**LuauAPI development will NOT support older versions of GD, Geode SDK, bindings, and dependencies.**

Mods can use Luau alone, combine C++ and Luau, or share APIs with other mods.

## Before you start

You are expected to already have:

- Basic Geode modding knowledge. [Learn Geode SDK here](https://docs.geode-sdk.org).
- A working knowledge of Lua or Luau scripting.
- Experience using VS Code.

*Optionally, you can learn these along the way.*

## Docs

Read the [documentation index](docs/README.md).
It links getting started, reference, and contributor docs.

Start here if you are new:

- [Overview](docs/getting-started/overview.md)
- [Installation](docs/getting-started/installation.md)
- [Editor setup](docs/getting-started/editor-setup.md)
- [Your first script](docs/getting-started/first-script.md)
- [Examples](docs/getting-started/examples.md)
- [Lua module index](docs/reference/lua/globals.md)
- [LuauAPI example mod template](https://github.com/ItMe12s/luauapi-example-mod)
- [LuauAPI mod guidelines](docs/mod_guidelines.md)

Join the [Discord](https://discord.gg/E8f6D6XqbW) for help.

## Example mod template

Start new projects from the [LuauAPI example mod template](https://github.com/ItMe12s/luauapi-example-mod).
It already includes the LuauAPI dependency, Luau resources, editor config, and first script.
See [Installation](docs/getting-started/installation.md) for the Geode CLI and Download ZIP setup paths.

### Example mod contributors

The local `luauapi-example-mod/` copy is a Git submodule. Include it when cloning LuauAPI:

```sh
git clone --recurse-submodules https://github.com/ItMe12s/LuauAPI.git
```

For an existing clone:

```sh
git submodule update --init --recursive
```

## Project

Special thanks to:

- [Juniper](https://github.com/TreehouseFalcon) (Testing, Debugging)
- [Erymanthus](https://github.com/RayDeeUx) (Testing, Debugging)
- [YellowCat98](https://github.com/YellowCat98) (Testing)

In-game:

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
- [JetBrains Mono](https://www.jetbrains.com/lp/mono/#license)
- [Luau](https://github.com/luau-lang/luau/blob/master/LICENSE.txt)
- [mbedTLS](https://github.com/Mbed-TLS/mbedtls/blob/development/LICENSE)
- [stb](https://github.com/nothings/stb/blob/master/LICENSE)
