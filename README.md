# Setup your visual studio

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
| `setMemoryLimit(bytes)`                                             | Sets shared VM allocator limit, clamped to supported range.          |
| `codegenEnabled()`                                                  | Whether Luau native codegen is active.                               |

`deadlineMs <= 0` disables the Luau CPU budget for that execution.
Default script deadline is `250 ms`. Generated hook callbacks use `50 ms`.
`setMemoryLimit` clamps to `16 MiB..512 MiB`. Default memory limit is `64 MiB`.
**Please use the default values** unless you're making a really heavy mod.
`lastError()` is a synchronous side channel. Read it before the next LuauAPI call. Reentrant calls can replace it.

Threading contract:

- All public APIs are main-thread only.
- Async APIs read and compile off-thread, then queue execution on the main thread.
- Off-thread query calls return default values and do not create the runtime.
- Off-thread `setMemoryLimit` does nothing.

Runtime contract:

- Panic is terminal for this process. Status becomes `Panicked`, readiness becomes false, and there is no in-process recovery path.
- OOM is a shared-pool allocation denial. The allocator returns null when the shared memory limit is exceeded. Status does not change to `Panicked`.
- Dependent mods must check `isReady()` or `status()` before execution and treat non-ready status as a shared platform failure.
- Native C++ work called from Luau is outside Luau interrupt budgeting.
- Geode hook registration is capped at 4096 live callbacks globally and 64 live callbacks per target. Removed callbacks stop counting after later registrations compact the registry.

Dependency contract:

- LuauAPI public headers use Geode API types such as `geode::Result` and `geode::Task`.
- Dependent mods must declare `imes.luauapi` in `mod.json`.
- Build dependency pins are intentionally unchanged in this release.

## Getting types

To get the `.d.luau` files, compile this mod on your pc.
Then copy the `types` folder into your mod project root.

**BTW IT TAKES LIKE FOREVER TO COMPILE**, if you don't have a workstation cpu.

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
