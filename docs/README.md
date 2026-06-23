# LuauAPI documentation

## Summary

This folder holds all LuauAPI docs. Pages are grouped by audience.
Start with the section that matches your goal.
Read [Documentation style](STYLE.md) before you edit or add pages.

LuauAPI is in beta. Expect API and doc changes.

## Getting started

For mod authors who depend on LuauAPI and run `.luau` scripts from a Geode mod.

Read [LuauAPI mod guidelines](mod_guidelines.md) before you ship.

| Page | Role |
| --- | --- |
| [Getting started](getting-started/overview.md) | Runtime basics and threading |
| [Installation](getting-started/installation.md) | Dependency and platform setup |
| [Editor setup](getting-started/editor-setup.md) | Autocomplete and stubs |
| [Your first script](getting-started/first-script.md) | First `runFile` from C++ |
| [Examples](getting-started/examples.md) | Runnable script samples |

## Reference

API reference for scripts and C++ hosts.
Start at [globals](reference/lua/globals.md) for the full Lua module index.

### Lua

Start at [globals](reference/lua/globals.md) for the full index and error shapes.

| Group | Page | Role |
| --- | --- | --- |
| Core | [hooks](reference/lua/hooks.md) | Hook game functions |
| Core | [modules](reference/lua/modules.md) | Sandboxed `require` |
| Core | [sharing APIs between mods](reference/lua/sharing-apis.md) | `_G` mod APIs |
| Core | [tasks and time](reference/lua/tasks.md) | `task` and `time` |
| Core | [callbacks](reference/lua/callbacks.md) | C++ callback lifetime |
| Core | [delegates](reference/lua/delegates.md) | Virtual interface tables |
| Game | [enums](reference/lua/enums.md) | GD and Geode enum constants |
| Game | [game objects](reference/lua/game-objects.md) | Cocos and GD objects |
| Game | [cocos](reference/lua/cocos.md) | Node and color helpers |
| Game | [ColorProvider](reference/lua/color-provider.md) | Theme colors |
| Game | [Keybind](reference/lua/keybind.md) | Keybind strings |
| Game | [Keyboard input](reference/lua/keyboard-input.md) | Keyboard events |
| UI | [UI and layouts](reference/lua/ui.md) | Cocos UI factories and layouts |
| UI | [imgui](reference/lua/imgui.md) | Dear ImGui overlay |
| UI | [gd3d](reference/lua/gd3d.md) | 3D viewport rendering |
| IO | [mod](reference/lua/mod.md) | Saved values and settings |
| IO | [fs](reference/lua/fs.md) | Sandboxed filesystem |
| IO | [json](reference/lua/json.md) | JSON parse and dump |
| Network | [web](reference/lua/web.md) | HTTP client |
| Network | [websocket](reference/lua/websocket.md) | WebSocket client and server |
| Utils | [geode.utils](reference/lua/utils.md) | Clipboard, string, random, and other small helpers |
| Utils | [game](reference/lua/game.md) | Game process control |
| Utils | [base64](reference/lua/base64.md) | Base64 encode and decode |
| Utils | [permission](reference/lua/permission.md) | OS permissions |
| Utils | [VersionInfo](reference/lua/version-info.md) | Version parsing |
| Meta | [type stubs](reference/lua/type-stubs.md) | `geode.d.luau` reference |

### C++

| Page | Role |
| --- | --- |
| [C++ API reference](reference/cpp/api-reference.md) | Run functions, threading, and integration |
| [Limits and errors](reference/cpp/limits-and-errors.md) | Caps, deadlines, and error strings |

## Contributor

For people who build LuauAPI from source or work on the runtime and codegen.

| Page | Role |
| --- | --- |
| [Architecture](contributor/architecture.md) | Runtime layout |
| [Building from source](contributor/building.md) | Build steps |
| [Testing](contributor/testing.md) | Test map |

Internals:

| Page | Role |
| --- | --- |
| [Bindings framework](contributor/internals/bindings-framework.md) | Binding layer |
| [ImGui draw scheduler](contributor/internals/imgui-draw-scheduler.md) | ImGui frame scheduling |
| [Module system](contributor/internals/module-system.md) | `require` and sandboxes |
| [Runtime](contributor/internals/runtime.md) | Lua runtime lifecycle |
| [Crash sidecar](contributor/internals/crash-sidecar.md) | Luau context at native faults |
| [Task scheduler](contributor/internals/task-scheduler.md) | Main-thread tasks |

Codegen:

| Page | Role |
| --- | --- |
| [Codegen](contributor/codegen/codegen.md) | Codegen overview |
| [CCArray methods](contributor/codegen/cc-array.md) | CCArray binding rules |
| [ccCArray read-only fields](contributor/codegen/cc-c-array.md) | ccCArray field rules |
| [Nested containers](contributor/codegen/nested-containers.md) | Nested container codegen |
| [Pair containers](contributor/codegen/pair-containers.md) | Pair container codegen |
| [Platform parity](contributor/codegen/platform-parity.md) | Cross-platform stubs |

## Policy and style

| Page | Role |
| --- | --- |
| [LuauAPI mod guidelines](mod_guidelines.md) | Mod author policy |
| [Documentation style](STYLE.md) | How to write docs |

## Related

- [Project README](../README.md)
- [Getting started](getting-started/overview.md)
- [globals](reference/lua/globals.md)
- [Codegen](contributor/codegen/codegen.md)

## Source

- `docs/`
