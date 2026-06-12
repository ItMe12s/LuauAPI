# LuauAPI

**Build Geode mods in Luau!**

LuauAPI is an ambitious scripting ecosystem for Geode,
making Luau a first-class way to create mods, APIs, tools,
and reusable systems without requiring every creator to work directly in C++. (1)

It includes a higher-level SDK that helps you start simple and iterate faster, without all the C++ shenanigans.
Plus, you get fewer crashes and clear, easy-to-understand error logging.

**READ THE PSA AND NOTES AT THE BOTTOM!!!**

---

## Features

- Fully cross-platform
- Write fully functional Geode mods in Luau (2)
- Luau LSP autocompletion support (3)
- Create your own API mods in Luau
- In-game developer tools (enable via developer mode)
- JIT and JIT-less support
- Easy-to-use 3D APIs and viewports through `gd3d`
- Rich built-in libraries, including `task`, `loadstring`, and more

---

## Upcoming

- Custom UI builder, framework, pre-made assets
- More advanced 3D functionality and support
- Sprite and rig animator
- The remaining 0.01% of bindings and m_Fields
- Other quality of life features

---

## Learn more

[>>> Check out the LuauAPI Documentation <<<](https://github.com/ItMe12s/LuauAPI/blob/master/README.md)

---

## PSA: NORMAL USERS READ THIS

If someone asks you to **turn on Developer Mode** and copy and paste a script, **DON'T DO IT**.
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

Limitations and when to use C++

- Heavy data processing and databases
- Low-level functionality and optimizations
- Multithreading
