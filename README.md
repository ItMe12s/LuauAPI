# LuauAPI

**Write Geode SDK mods with Luau!**

Before you start, Luau doesn't mean you can just not do memory management.
You'll still have to write performant code and now with Luau typing on top.
Also don't vibecode (duh).

## Platform bindings

Generated API differs per platform. Build for your target before copying `types/`.

| Platform       | Bindings                                |
| -------------- | --------------------------------------- |
| **Windows**    | Full API + `geode.hook`                 |
| **Android**    | Generated calls + `geode.hook` targets  |
| **iOS/macOS**  | ~None (Apple `link` filter applies)     |

Cross-platform Luau scripts are not yet supported. Android hook targets are generated per ABI.
Manual Android device testing is still required before treating a hook-heavy script as release-ready.

- Windows uses `win 0x...` addresses. (Full support, make anything you want).
- Android uses `[[link(android)]]`, `android32`, and `android64` symbols.
  - Generated calls and `geode.hook` targets are ABI-specific, so build Android32 or Android64 before copying `types/`.
- iOS/macOS only emit `link`-tagged symbols. (You ain't doing shit except basic Luau stuff).

## Setup your visual studio

You would want autocomplete and help from the lsp.

Use the `Luau Language Server` extension by JohnnyMorganz.

## C++ API

LuauAPI is a shared Geode dependency mod. It owns one singleton Luau VM for every dependent mod in the process.

Add LuauAPI as a required dependency in your mod:

```json
{
    "dependencies": {
        "imes.luauapi": ">=v1.0.0"
    }
}
```

Use the public header from C++:

```cpp
#include <imes.luauapi/LuauAPI.hpp>
```

Main thread is required for every public API. Async APIs read and compile off-thread, then queue execution on the main thread.

Available calls:

| Function                                                            | Purpose                                                              |
| ------------------------------------------------------------------- | -------------------------------------------------------------------- |
| `runFile(resourcesRoot, relativePath, deadlineMs = 250)`            | Run a packaged flat `.luau` resource file.                           |
| `runScript(resourcesRoot, source, chunkName, deadlineMs = 250)`     | Run source text with a flat virtual chunk name.                      |
| `runFileAsync(resourcesRoot, relativePath, deadlineMs = 250)`       | Async file read and compile, main-thread execution.                  |
| `runScriptAsync(resourcesRoot, source, chunkName, deadlineMs = 250)`| Async compile, main-thread execution.                                |
| `isReady()`                                                         | Compatibility readiness check.                                       |
| `status()`                                                          | Returns `NotReady`, `Ready`, `InitFailed`, or `Panicked`.            |
| `lastError()`                                                       | Returns last synchronous runtime error string.                       |
| `memoryUsage()`                                                     | Shared VM allocator usage in bytes.                                  |
| `memoryLimit()`                                                     | Shared VM allocator limit in bytes.                                  |
| `codegenEnabled()`                                                  | Whether Luau native codegen is active.                               |

- `deadlineMs <= 0` disables the Luau CPU budget for that execution.
- Default script deadline is `250 ms`. Generated hook callbacks use `50 ms`.
- Memory limit is `512 MiB`.
- `lastError()` only shows the latest sync error, check before your next call.

**Threading contract:**

- All public APIs are main-thread only.
- Async APIs run off-thread, then execute on the main thread.
- Off-thread queries return defaults and don't start the runtime.

**Runtime contract:**

- Panic is fatal, status changes to `Panicked` with no recovery.
- OOM means hitting the memory limit, allocator returns null, status stays unchanged.
- Runtime loads at startup. `isReady()` and `status()` exist for diagnostics but checking before use is not required.
- Native C++ called from Luau isn't budgeted.
- 4096 global max hooks and 64 per target. Removed hooks don't count after next registry compact.

**Dependency contract:**

- LuauAPI public headers use Geode API types such as `geode::Result` and `geode::Task`.
- Build dependency pins are intentionally unchanged in this release.

## Getting types

To get the `.d.luau` files, compile this mod on your PC.
Then copy the `types` folder into your mod project root.

**BTW IT TAKES LIKE FOREVER TO COMPILE** if you don't have a workstation cpu.

Also do the vscode `>Developer: Restart Extension Host` after doing stuff with luau lsp.

## Setting files

Add this to your `.vscode/settings.json`
It's split into 3 files because luau lsp can't just process 13k lines at once.

```json
{
    "luau-lsp.platform.type": "standard",
    "luau-lsp.types.definitionFiles": {
        "@geode-cocos2d": "types/geode_cocos2d.d.luau",
        "@geode-gd": "types/geode_gd.d.luau",
        "@geode": "types/geode.d.luau"
    },
    "luau-lsp.ignoreGlobs": [
        "**/*.d.luau"
    ]
}
```

At your project root, add this `.luaurc` file

```json
{
    "languageMode": "strict",
    "aliases": {
        "types": "types"
    }
}
```

## What it should look like

Since you can't do folders inside resources, you'll just have to name and prefix them properly.

```.
YourMod
|- .vscode
|   |- settings.json
|
|- mod
|   |- Bootstrap.luau
|   |- mod_YourModule.luau
|   |- hook_GameLayer.luau
|   |- CoolScript.luau
|
|- src
|   |- main.cpp
|
|- types
|   |- geode_cocos2d.d.luau
|   |- geode_gd.d.luau
|   |- geode.d.luau
|
|- .luaurc
|
|... and everything else from the Geode mod template.
```

## Don't forget to include the files

Add `Bootstrap.luau` to your `mod.json` resources so Geode packages it as a runtime file:

```json
{
    "resources": {
        "files": [
            "mod/*.luau"
        ]
    }
}
```

`runFile(Mod::get()->getResourcesDir(), "Bootstrap.luau")` expects the script as a flat resource name.

## Development checks

To run optional host tests, build with `-DLUAUAPI_BUILD_TESTS=ON` and run `luauapi_tests` or CTest.
Manual Geode testing is still needed per usual.

## Extra info

The broma parser code is from my previous project saphhire sdk,
which brings Luau like syntax to Geode (weren't very useful afaik).
