# LuauAPI

**Build Geode mods in Luau!**

LuauAPI is an ambitious scripting ecosystem for Geode,
making Luau a first-class way to create mods, APIs, tools,
and reusable systems without requiring every creator to work directly in C++. (1)

It includes a higher-level SDK that helps you start simple, iterate faster, and share custom Luau APIs across mods.

---

## Features

- Write fully functional Geode mods in Luau (2)
- Luau LSP autocompletion (3)
- Create your own API mod in Luau
- In-game developer tools (enable via developer mode)
- JIT and JIT-less support
- 3D rendering and viewports through `gd3d`
- Rich built-in libraries, including `task`, `loadstring`, and more

---

## PSA: NORMAL USERS READ THIS

If someone asks you to **turn on Developer Mode** and copy-paste a script, **DON'T DO IT**.
They're trying to trick you. Running untrusted scripts can:

- Delete your save files
- Steal your account password
- Download viruses onto your computer or phone

LuauAPI is not responsible for any damages caused by unverified scripts.
Only download mods from the official [Geode SDK](https://geode-sdk.org/) index or in-game.

---

## Learn more

[>>> Check out the LuauAPI Documentation <<<](https://github.com/ItMe12s/LuauAPI/blob/master/README.md)

---

## Notes

1. A C++ environment/loader is still required to bootstrap and load the Luau scripts.
2. Certain functionality may be sandboxed or restricted for security and stability.
3. Requires the Luau Language Server VSCode extension by JohnnyMorganz with some setups.

Limitations and when to use C++

- Heavy data processing and databases
- Low-level functionality and optimizations
- Advanced networking and multi-threaded tasks
