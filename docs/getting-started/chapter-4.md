# Chapter 4: Your first script

## Summary

This page walks through the smallest working setup. You depend on LuauAPI, write a Luau file, then run it from a host mod.

## Step 1: depend on LuauAPI

LuauAPI exports its public header, and the mod exposes `include/**/*.hpp` as its API in `mod.json`.
Add a dependency on `imes.luauapi` in your own mod, then include the header.

```json
{
    "dependencies": {
        "imes.luauapi": ">=0.1.0-beta.1"
    },
    "resources": {
        "files": [
            "mod/*.luau"
        ]
    }
}
```

Put your `.luau` files under the resources path you declare. Pack them with your mod.

```cpp
#include <imes.luauapi/include/LuauAPI.hpp>
namespace lua = imes::luauapi;
```

All public functions live in the `imes::luauapi` namespace.

LuauAPI loads early with first priority. Check `status()` is `Ready` before you call `runFile` if needed.

## Step 2: write the script

Create a `.luau` file in your mod resources and name it `Bootstrap.luau`.

```lua
print("Hello from Luau")
```

`print` writes to the Geode log, and each argument is separated by a tab.

## Step 3: run it from C++

Run the file from a mod that depends on LuauAPI. Use `runFile` with your resources directory and the file name.

```cpp
#include <Geode/Geode.hpp>
#include <Geode/loader/ModEvent.hpp>
#include <imes.luauapi/include/LuauAPI.hpp>

using namespace geode::prelude;
namespace lua = imes::luauapi;

$on_mod(Loaded) {
    queueInMainThread([] {
        auto result = lua::runFile(Mod::get()->getResourcesDir(), "Bootstrap.luau");
        if (result.isErr()) {
            log::error("script failed: {}", result.unwrapErr());
        }
    });
}
```

`runFile` must run on the main thread. The example uses `queueInMainThread` to make sure of that.

## What the rules are

The file name must be a flat `.luau` resource name inside the resources directory you pass.
No folders, no `..`, no absolute paths, and it must be within the size limit.
See [Modules and require](../lua/guide/modules-and-require.md) and [Limits and errors](../cpp/limits-and-errors.md).

## Developer mode

LuauAPI also ships built-in developer tools. They load when you turn on developer mode in the mod settings.
Developer mode is off by default. Turn it on only if you need these tools.

## Related

- Script side: [Overview](../lua/overview.md)
- Host side: [Integration guide](../cpp/integration-guide.md)

## Next

- [Chapter 5: Core concepts](chapter-5.md)

Back to [Chapter 3: Editor setup](chapter-3.md).

## Source

- `src/api.cpp`
- `include/LuauAPI.hpp`
- `src/lua/runtime/Runtime.cpp`
