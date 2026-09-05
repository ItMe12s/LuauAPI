# LuauAPI

One shared VM for all your Geode mods that uses Luau.

Write full mods, tools, and APIs in Luau without the C++ stuff. (1)

Easy to start, fast to compile, with clear error logging when something breaks.

**READ THE PSA AND NOTES AT THE BOTTOM!!!**

---

## Currently implemented

- Fully cross-platform on Windows, Android, iOS and macOS
- Real mods written fully in Luau using hooks, callbacks, delegates, saved values and settings (2)
- ~99.9% binding coverage of GD and cocos classes generated from Geode's Broma bindings
- Cross-mod APIs published under your mod id, and C++ mods can expose functions and values too
- Rich built-in libraries like `task`, `imgui`, `websocket` client/server, `json`, `fs`, HTTP, keybinds and enums
- 3D viewports, glTF meshes and materials through `gd3d`
- Sprite rigging and animation through `lunar`
- Sandboxed with deadlines, memory caps and file access. Errors get logged instead of crashing the game
- In-game developer tools including a live script executor
- Luau LSP autocompletion and strict mode type checking for VSCode (3)
- JIT and JIT-less support

---

## Coming next

- Custom UI builder, framework, pre-made assets
- More advanced 3D functionality and support
- The remaining 0.1% of bindings and m_Fields
- Argon support
- Box2D support
- SQLite support
- In-game IDE
- Other QoL features

---

## Learn more

[>>> Read the LuauAPI Documentation <<<](https://github.com/ItMe12s/LuauAPI/blob/master/docs/README.md)

Starting a new project? Use the [example mod template](https://github.com/ItMe12s/luauapi-example-mod).

Need help? Join the [Discord](https://discord.gg/E8f6D6XqbW).

---

## PSA: NORMAL USERS, READ THIS

### Making a crash report

First turn on **Enable Crash Context File** in LuauAPI's mod settings under User Settings.

Debugging only!!! it can cause lag, so leave it off after you are done.

Then crash the game. In the `Geode Crash Handler` window click `Open crashlog folder`.

Send both files to the developers, the `luauapi-last-context.txt` sidecar file and the Geode crashlog.

`luauapi-last-context.txt` gets overwritten on the next launch, so copy it somewhere safe before restarting.

### Never run scripts from strangers

If someone asks you to **turn on Developer Mode and execute a script, DON'T.**

They're trying to trick you. Running untrusted scripts can:

- Delete your save files
- Steal your account password
- Download viruses onto your computer or phone

LuauAPI is not responsible for any damages caused by unverified scripts.
Only download mods from the official [Geode SDK](https://geode-sdk.org/) index or in-game.

---

## Notes

1. A C++ environment/loader is still required to bootstrap and load the Luau scripts.
2. Certain functionality may be sandboxed or restricted for security and stability.
3. Requires the Luau Language Server VSCode extension by JohnnyMorganz with some setups.

### When to still use C++

For now, C++ is still the tool for:

- Heavy data processing and databases
- Low-level functionality and optimizations
- Multithreading

Work is ongoing to shrink this list. These will be solved in the future :3