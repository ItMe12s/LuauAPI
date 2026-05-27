# LuauAPI

**Write Geode mods in Luau.**

This is a Geode mod other mods depend on.
It runs **one shared Luau engine** for the whole game and gives you typed wrappers for GD, Cocos2d, and Geode.

## Heads up

- Luau does **not** free you from caring about memory and speed.
- Types help. Bad code is still bad code.
- Don't vibecode your whole mod (duh, also this isn't in any training data yet).

---

## Quick start

### 1. Dependency

Put this in your mod's `mod.json`:

```json
{
    "dependencies": {
        "imes.luauapi": ">=v1.0.0"
    }
}
```

### 2. Call it from C++ on the main thread

Not on a random thread. Use `queueInMainThread` if you need to.

```cpp
#include <imes.luauapi/include/LuauAPI.hpp>
```

```cpp
queueInMainThread([] {
    imes::luauapi::runFile(
        Mod::get()->getResourcesDir(),
        "Bootstrap.luau"
    );
});
```

Tiny working sample: [`example/minimalMain.cpp`](example/minimalMain.cpp) + [`example/Bootstrap.luau`](example/Bootstrap.luau).

### 3. Put your `.luau` files in `mod/`

List them in `mod.json` so Geode packs them. See [Recommended mod layout](#recommended-mod-layout).

### 4. Run scripts by filename only

```cpp
runFile(resourcesRoot, "CoolScript.luau");
```

No folders in the path. Just the file name. More below.

### 5. Get autocomplete types

Build LuauAPI once. Copy the `types/` folder into your mod. Same files work on every platform. [How](#getting-types).

### 6. Set up VS Code

Copy the config from [Editor setup](#editor-setup). Add `types/` to `.gitignore` in **your** mod.

### 7. Install Luau Language Server

Get **Luau Language Server** by JohnnyMorganz.

### 8. Actually test in the game

LuauAPI won't test for you.
But it offers instant code compilation (Luau handles that).

---

## Writing Luau scripts

### The three namespaces you'll use

| Name | What's in it |
| ---- | -------------- |
| `geode` | Hooks + Geode stuff |
| `geode.gd` | Geometry Dash game classes |
| `geode.cocos2d` | UI / sprites / labels |

### Hooks (change game code when something runs)

```luau
local hook = geode.hook("geode.gd.MenuLayer:init/0", function(self)
    -- your code
end)
```

- First argument is a string like `"Class:method/howManyArgs"`.
- Hooks get **50 ms** of CPU time per run.
- Normal scripts get **250 ms** by default.
- Max **4096** hooks total, **64** on one target.

### Stuff you can't (and shouldn't) do in LuauAPI

- You **can't** read raw `m_*` memory fields from Broma.
- Use normal methods, hooks, or C++ `$modify` instead.
- If a hook or script cost more than **50-250 ms** to run then this mod probably isn't for the job.
- You should do heavy data processing and networking in C++ instead (e.g. Parsing level string, Argon auth).

### File names (important)

Geode won't pack scripts in subfolders. One file = one name in the folder.

| OK | Not OK |
| -- | ------ |
| `hook_GameLayer.luau` | `mod/hooks/GameLayer.luau` |
| `mod_CoolScript.luau` | `stuff/deep/script.luau` |

Use prefixes to group files (`hook_`, `mod_`).

### `require` (load another script)

- Looks in the same folder you gave `runFile`.
- Same flat-name rules.
- Only works from a loaded script name like `@Bootstrap.luau`.

### Copy-paste examples

[`example/`](example/) is **not built** with the project. Steal code from it.

| Files | What it shows |
| ----- | ------------- |
| [`minimalMain.cpp`](example/minimalMain.cpp) + [`Bootstrap.luau`](example/Bootstrap.luau) | Smallest setup |
| [`main.cpp`](example/main.cpp) + [`MainThreadBootstrap.luau`](example/MainThreadBootstrap.luau) | Run on main thread + hook UI |
| [`main.cpp`](example/main.cpp) + [`AsyncBootstrap.luau`](example/AsyncBootstrap.luau) | Load file in background, run on main thread |

---

## C++ API (what you can call)

One Luau engine for **all** mods that depend on this. It starts early so it's ready before your mod loads.

**Rule #1. Call everything on the main thread.**

"Async" means read and compile on another thread. The script still **runs** on the main thread.

### Functions

| Function | What it does |
| -------- | ------------ |
| `runFile(root, "file.luau", 250)` | Run a `.luau` file from your mod resources |
| `runScript(root, source, "name.luau", 250)` | Run text you pass in |
| `runFileAsync(...)` | Read file in background, run on main thread |
| `runScriptAsync(...)` | Compile in background, run on main thread |
| `isReady()` | `true` if runtime is good to go |
| `status()` | `NotReady`, `Ready`, `InitFailed`, or `Panicked` |
| `lastError()` | Last error message (string) |
| `memoryUsage()` | How much RAM the VM uses (bytes) |
| `memoryLimit()` | Max RAM allowed (bytes) |
| `codegenEnabled()` | Is speed-up codegen on? |

### Numbers to remember

| Thing | Value |
| ----- | ----- |
| Default script time limit | **250 ms** |
| Hook time limit | **50 ms** |
| Max VM memory | **512 MiB** |
| Pass `0` for deadline | No time limit for that run |
| Max hooks (all) | **4096** |
| Max hooks (one target) | **64** |

### Errors (two places)

- `runFile` returns `geode::Result`. Check `.isErr()`.
- `lastError()` is a separate string. Read it **before** your next LuauAPI call.

### Main thread again

```cpp
queueInMainThread([] {
    imes::luauapi::runFile(Mod::get()->getResourcesDir(), "Bootstrap.luau");
});
```

### If things go wrong

| Problem | What happens |
| ------- | -------------- |
| **Panic** | Game VM is dead (`Panicked`). No fix. |
| **Out of memory** | Hit 512 MiB cap. Alloc fails. |
| Call API off main thread | Wrong/default answers. Runtime won't start. |
| C++ you call from Luau | Not counted in the ms time limit |

You usually **don't** need to check `isReady()` every time. It loads at startup.

---

## Types (same on every platform)

LuauAPI builds the type list from stuff that exists on **all** of these:

- Windows (`win`)
- Mac ARM (`m1`)
- iOS (`ios`)
- Android 32 bit (`android32`)
- Android 64 bit (`android64`)

If a method or hook is missing on one of those, it will **not** show up in your types. That is on purpose. Your editor matches what every platform can actually run.

You do **not** need to rebuild types per platform. Build LuauAPI once, copy `types/` once, done.

### Getting types

1. Build LuauAPI like any Geode mod (any platform build is fine for types).
2. Copy the whole `types/` folder into your mod root.
3. Commit or gitignore that folder in your mod (most people gitignore it).

**Compile takes forever** without a beefy CPU. Sorry!!!

After copying types, in VS Code run **`Developer: Restart Extension Host`**.

### Still test in the real game

Types being the same does not mean you can skip testing.

- Test on Android on a real phone if your mod uses hooks.
- Test on iOS / Mac if you ship there.

If something breaks on one platform, it was probably never in the shared type list, or it is a runtime issue, not a wrong types file.

If you hack on LuauAPI itself, open `luauapi-gen/report.md` in the build folder to see what codegen dropped and why.

---

## Editor setup

Install **Luau Language Server** (JohnnyMorganz).

Types are split into several files because one huge file breaks the LSP.

**`.vscode/settings.json`**

Put this in the file:

```json
{
    "luau-lsp.platform.type": "standard",
    "luau-lsp.types.definitionFiles": {
        "@geode-cocos2d": "types/geode_cocos2d.d.luau",
        "@geode-cocos2d-factories": "types/geode_cocos2d_factories.d.luau",
        "@geode-gd": "types/geode_gd.d.luau",
        "@geode-gd-2": "types/geode_gd_2.d.luau",
        "@geode-gd-factories": "types/geode_gd_factories.d.luau",
        "@geode": "types/geode.d.luau"
    },
    "luau-lsp.ignoreGlobs": [
        "**/*.d.luau"
    ]
}
```

**`.luaurc` in your mod root**

Put this in the file:

```json
{
    "languageMode": "strict",
    "aliases": {
        "types": "types"
    }
}
```

---

## Recommended mod layout

No nested script folders. Use name prefixes.

```text
YourMod
|- .vscode/settings.json
|- mod
|   |- Bootstrap.luau
|   |- mod_YourModule.luau
|   |- hook_GameLayer.luau
|- src/main.cpp
|- types/
|   |- (copied from LuauAPI build)
|- .luaurc
```

**Pack scripts in `mod.json`**

```json
{
    "resources": {
        "files": [
            "mod/*.luau"
        ]
    }
}
```

**Run them like this.** Do not put `mod/` in the string:

```cpp
runFile(Mod::get()->getResourcesDir(), "Bootstrap.luau");
```

**`.gitignore` in your mod**

```gitignore
types/
```

---

## Working on LuauAPI itself (contributors)

### You need

- Whatever Geode SDK needs.
- Python 3.11+

### Build + tests

Normal Geode mod build.

Want unit tests? Configure with:

```text
-DLUAUAPI_BUILD_TESTS=ON
```

Then build `luauapi_tests` and run `ctest`.

Also run test `luauapi_codegen_hooks` (Python script in `tests/`).

CI builds Win, Mac, iOS, Android32, and Android64. See [`.github/workflows/multi-platform.yml`](.github/workflows/multi-platform.yml).

### Where stuff lives

| Folder | What's there |
| ------ | ------------ |
| `include/` | Header other mods include |
| `src/` | Main mod + API code |
| `src/lua/` | VM, require, paths |
| `tools/luau_codegen/` | Generates bindings + types |
| `tests/` | Fast tests without full game |
| `example/` | Samples for mod authors |

Codegen script is [`tools/luau_codegen/codegen.py`](tools/luau_codegen/codegen.py). CMake target is `luauapi_codegen`.

### History

Parser code started in an older project called Sapphire SDK (not very useful afaik).

The Luau wrapper design came from my FishRNG mod, where I tried to use Luau so I don't have to touch C++ codes.
