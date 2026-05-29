# LuauAPI

**Write Geode mods in Luau.**

This is a Geode mod other mods depend on.
It runs **one shared Luau engine** for the whole game and gives you typed wrappers for GD, Cocos2d, and Geode.

## Heads up

- Luau does **not** free you from caring about memory and speed.
- Types help. Bad code is still bad code.
- Don't vibecode your whole mod (duh, also this isn't in any training data yet).
- [Read Mod Guidelines](https://docs.geode-sdk.org/mods/guidelines).
- [Read Guidelines Tips](https://docs.geode-sdk.org/mods/guidelines-tips).

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

| Name            | What's in it               |
| --------------- | -------------------------- |
| `geode`         | Hooks + Geode stuff        |
| `geode.gd`      | Geometry Dash game classes |
| `geode.cocos2d` | UI / sprites / labels      |

### Hooks (change game code when something runs)

```luau
local hook = geode.hook("geode.gd.MenuLayer:init/0", {
    priority = 0,
    before = function(self)
        self.m_fields.calls = (self.m_fields.calls or 0) + 1
    end,
    after = function(self, result)
        return result
    end,
})
```

- First argument is a string like `"geode.gd.ClassName:method/N"` or `"geode.cocos2d.ClassName:method/N"`.
- Second argument must be a callback table. Bare function hooks are rejected.
- `geode.modify(...)` is the same API with Geode-style naming.
- `before` runs before original. Return `{ args = { ... } }` to replace args.
- Return `geode.skip(value)` from `before` to skip original and use `value` as result.
- `after` runs after original or skipped result. Returning a value overrides result.
- Lower `priority` runs earlier for `before`, higher `priority` runs earlier for `after`.
- Hooks get **50 ms** of CPU time per run.
- Normal scripts get **250 ms** by default.
- Max **4096** hook callbacks total, **64** on one target. A hook with both `before` and `after` counts as 2.

### Broma fields and `m_fields`

- Supported Broma fields are plain properties: `node.m_obPosition`, `node.m_nTag`.
- Writes use same checks as method args: `node.m_obPosition = { x = 10, y = 20 }`.
- Missing SDK fields and unsafe field shapes are skipped by codegen.
- `self.m_fields` stores Lua scratch state on `CCNode` instances and descendants only.
- `geode.fields(self)` returns same table as `self.m_fields`.
- Scratch state lifetime follows underlying `CCNode`.

### Stuff you can't (and shouldn't) do in LuauAPI

- If a hook or script cost more than **50-250 ms** to run then this mod probably isn't for the job.
- You should do heavy data processing and networking in C++ instead (e.g. Parsing level string, Argon auth).
- Don't make custom libraries yet. There will be built-in libraries for stuff like Task, ImGui, Argon and more later.

### File names (important)

Geode won't pack scripts in subfolders. One file = one name in the folder.

| **OK**                 | **Not OK**                 |
| ---------------------- | -------------------------- |
| `hook_GameLayer.luau`  | `mod/hooks/GameLayer.luau` |
| `mod_CoolScript.luau`  | `stuff/deep/script.luau`   |

Use prefixes to group files (`hook_`, `mod_`).

### `require` (load another script)

- Looks in the same folder you gave `runFile`.
- Same flat-name rules.
- Only works from a loaded script name like `@Bootstrap.luau`.

### `task` and `time` (timers)

Two built-in globals for scheduling tasks over frames. Callback-based, no yielding. Times are in seconds (float).

`task`:

| Call                  | Description                                        |
| --------------------- | -------------------------------------------------- |
| `task.spawn(fn, ...)` | Run `fn` immediately (args forwarded).             |
| `task.delay(sec, fn)` | Run `fn` after `sec` seconds (returns handle).     |
| `task.every(sec, fn)` | Run `fn` every `sec` seconds (returns handle).     |
| `task.defer(fn)`      | Run `fn` on next frame (returns handle).           |
| `task.cancel(handle)` | Cancel scheduled task (same as `handle:cancel()`). |

`delay`, `every`, and `defer` return a handle. Use `handle:cancel()` or `task.cancel(handle)` to stop it,
doing this more than once is safe.
These functions only take `(time, fn)` as arguments (only `spawn` lets you pass extra arguments).

Timers run with the game (pause, timeScale). Errors stop the task. Max `kMaxScheduledTasks` (4096), exceeding this errors.

`time`:

| Call          | Description                                  |
| ------------- | -------------------------------------------- |
| `time.now()`  | Monotonic seconds since the runtime started. |
| `time.unix()` | Wall-clock seconds since the Unix epoch.     |

See [`TaskBootstrap.luau`](example/TaskBootstrap.luau).

### Copy-paste examples

[`example/`](example/) is **not built** with the project. Steal code from it.

| Files                                                          | What it shows                          |
| -------------------------------------------------------------- | -------------------------------------- |
| [`minimalMain.cpp`](example/minimalMain.cpp)                   | Smallest setup                         |
| [`Bootstrap.luau`](example/Bootstrap.luau)                     | Script file for minimal bootstrap      |
| [`main.cpp`](example/main.cpp)                                 | Entry that loads all example scripts   |
| [`MainThreadBootstrap.luau`](example/MainThreadBootstrap.luau) | Run on main thread + hook UI           |
| [`AsyncBootstrap.luau`](example/AsyncBootstrap.luau)           | `runFileAsync` from main thread        |
| [`HookModifyBootstrap.luau`](example/HookModifyBootstrap.luau) | Modify-style before/after hooks        |
| [`TaskBootstrap.luau`](example/TaskBootstrap.luau)             | `task` timers + `time` clock           |

[`main.cpp`](example/main.cpp) loads all three bootstrap scripts. Examples only show `runFile` and `runFileAsync`. See the C++ API table for `runScript` and `runScriptAsync`.

---

## C++ API (what you can call)

One Luau engine for **all** mods that depend on this. It starts early so it's ready before your mod loads.

**Rule #1. Scripts must run on the main thread.** Use `runFile` and `runScript` from the main thread or from `queueInMainThread`.

You can call async functions off the main thread. `runFileAsync` reads the file on your thread. Compile and run always happen on the main thread.

### Functions

| Function                                    | What it does                                               |
| ------------------------------------------- | ---------------------------------------------------------- |
| `runFile(root, "file.luau", 250)`           | Run a `.luau` file from your mod resources                 |
| `runScript(root, source, "name.luau", 250)` | Run text you pass in                                       |
| `runFileAsync(...)`                         | Read file on caller thread. Compile and run on main thread |
| `runScriptAsync(...)`                       | Schedule compile and run on main thread. Returns a Future  |
| `isReady()`                                 | `true` if runtime is good to go                            |
| `status()`                                  | `NotReady`, `Ready`, `InitFailed`, or `Panicked`           |
| `lastError()`                               | Last error message (string)                                |
| `memoryUsage()`                             | How much RAM the VM uses (bytes)                           |
| `memoryLimit()`                             | Max RAM allowed (bytes)                                    |
| `codegenEnabled()`                          | Is speed-up codegen on?                                    |

### Numbers to remember

| Thing                           | Value                      |
| ------------------------------- | -------------------------- |
| Default script time limit       | **250 ms**                 |
| Hook time limit                 | **50 ms**                  |
| Max VM memory                   | **512 MiB**                |
| Pass `0` for deadline           | No time limit for that run |
| Max hook callbacks (all)        | **4096**                   |
| Max hook callbacks (one target) | **64**                     |

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

| Problem                  | What happens                                        |
| ------------------------ | --------------------------------------------------- |
| **Panic**                | Game VM is dead (`Panicked`). No fix.               |
| **Out of memory**        | Hit 512 MiB cap. Alloc fails.                       |
| Call API off main thread | Sync calls fail. Most getters return empty/default. |
| C++ you call from Luau   | Not counted in the ms time limit.                   |

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

### Generated type files

Codegen writes these files into LuauAPI's `types/` folder (and into your mod after you copy them):

| File                             | Contents                                             |
|----------------------------------|----------------------------------------------------- |
| `geode.d.luau`                   | `geode` namespace: hooks, `modify`, `skip`, `fields` |
| `geode_cocos2d.d.luau`           | Cocos2d class declarations                           |
| `geode_cocos2d_factories.d.luau` | Cocos2d factory and namespace types                  |
| `geode_gd.d.luau`                | Geometry Dash class declarations (chunk 1)           |
| `geode_gd_2.d.luau`              | GD classes (chunk 2)                                 |
| `geode_gd_3.d.luau`              | GD classes (chunk 3)                                 |
| `geode_gd_4.d.luau`              | GD classes (chunk 4)                                 |
| `geode_gd_factories.d.luau`      | GD factory and namespace types                       |

GD class files are split into smaller files for the language server. If there are more classes, new files like `geode_gd_5.d.luau` will appear automatically. Old unused split files are removed on the next build.

Copy **all** `geode*.d.luau` files after a build, not a subset.

To list the exact filenames your bindings produce:

```bash
python tools/luau_codegen/codegen.py --bindings <path-to-bindings> --list-type-outputs --platform win --geode-sdk <GEODE_SDK>
```

Use `--list-outputs` with the same `--platform` as your Geode build when you need the C++ binding file list. `--list-type-outputs` uses the shared cross-platform API list, so the type file list is the same no matter which platform you pick.

### Getting types

1. Build LuauAPI like any Geode mod (any platform build is fine for types).
2. Copy the whole `types/` folder into your mod root (all `geode*.d.luau` files listed above).
3. Commit or gitignore that folder in your mod (most people gitignore it).

**Compile takes forever** without a beefy CPU. Sorry!!!

After copying types, in VS Code run **`Developer: Restart Extension Host`**.

### Still test in the real game

Types being the same does not mean you can skip testing.

- Test on Android on a real phone if your mod uses hooks.
- Test on iOS / Mac if you ship there.

If something breaks on one platform, it was probably never in the shared type list, or it is a runtime issue, not a wrong types file.

If you hack on LuauAPI itself, see [Codegen outputs](#codegen-outputs-luauapi-gen) and open `luauapi-gen/report.md` in the build folder to see what codegen dropped and why.

---

## Editor setup

Install **Luau Language Server** (JohnnyMorganz).

Types are split into multiple files to avoid breaking the LSP. Make sure to register every `geode*.d.luau` file you copied, including any new `geode_gd_N.d.luau` files by adding a matching `@geode-gd-N` entry below.

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
        "@geode-gd-3": "types/geode_gd_3.d.luau",
        "@geode-gd-4": "types/geode_gd_4.d.luau",
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
|   |- geode.d.luau
|   |- geode_cocos2d.d.luau
|   |- geode_cocos2d_factories.d.luau
|   |- geode_gd.d.luau
|   |- geode_gd_2.d.luau
|   |- geode_gd_3.d.luau
|   |- geode_gd_4.d.luau
|   |- geode_gd_factories.d.luau
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

- Whatever Geode SDK needs (Geode **5.7.1+**).
- `GEODE_SDK` env var pointing at your Geode SDK folder.
- CMake **3.21+** and a C++23 compiler.
- Python **3.11+**.
- Luau **0.722** and GD bindings **2.2081** are fetched automatically during configure.

### Build + tests

Normal Geode mod build.

Want unit tests? Configure with:

```text
-DLUAUAPI_BUILD_TESTS=ON
```

Then build `luauapi_tests` and run `ctest`. When `LUAUAPI_BUILD_TESTS=ON`, also run test `luauapi_codegen_hooks` (Python script in `tests/`).

CI builds Win, Mac, iOS, Android32, and Android64. See [`.github/workflows/multi-platform.yml`](.github/workflows/multi-platform.yml).

### Codegen outputs (`luauapi-gen`)

Building LuauAPI creates files in `${CMAKE_BINARY_DIR}/luauapi-gen/` for development and debugging. Dependent mods don't need these files.

| Path (under `build/.../luauapi-gen/`) | Purpose                                                                          |
| ------------------------------------- | -------------------------------------------------------------------------------- |
| `report.md`                           | Human-readable summary of skipped methods, intersection stats, and emitted paths |
| `schema.json`                         | Full parsed Broma metadata and which fields are bound                            |
| `parity.json`                         | Cross-platform parity data for CI and tooling                                    |
| `src/bindings_internal.hpp`           | Shared binding internals                                                         |
| `src/bindings_common.cpp`             | Common registration and shared glue                                              |
| `src/bindings_<ClassName>.cpp`        | One Luau binding translation unit per emitted class (count varies)               |

Luau type stubs for mod authors are written separately to the repo-root [`types/`](types/) folder (gitignored here, produced on build).

The binding file list is dynamic. List it after configure or with:

```bash
python tools/luau_codegen/codegen.py --bindings <path-to-bindings> --list-outputs --platform win --geode-sdk <GEODE_SDK>
```

CMake runs `--list-outputs` and `--list-type-outputs` at configure time so Ninja only rebuilds when generated files change.

### Where stuff lives

| Folder                | Contents / Purpose                                         |
| --------------------- | ---------------------------------------------------------- |
| `include/`            | Headers for other mods to use                              |
| `src/`                | Main mod and API source code                               |
| `src/lua/`            | VM code, `require`, path logic                             |
| `types/`              | Generated `geode*.d.luau` stubs (copy into dependent mods) |
| `tools/luau_codegen/` | Binding and type generators                                |
| `tests/`              | Fast/Python tests (no full game)                           |
| `example/`            | Example mods for authors                                   |

Build output only (not in the repo): `build/.../luauapi-gen/` holds generated C++ bindings, `report.md`, and JSON. See [Codegen outputs](#codegen-outputs-luauapi-gen).

Codegen script is [`tools/luau_codegen/codegen.py`](tools/luau_codegen/codegen.py). CMake target is `luauapi_codegen`.

### History

Parser code started in an older project called Sapphire SDK (not very useful afaik).

The Luau wrapper design came from my FishRNG mod, where I tried to use Luau so I don't have to touch C++ codes.
