# C++ integration guide

## Summary

This page shows how another Geode mod uses LuauAPI to run scripts from C++.
You depend on the mod, include the header, and call the run functions on the main thread.

## Depend on the API

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

## You do not start the runtime

The LuauAPI mod owns the runtime lifecycle. It records the main thread at load, creates the runtime when its mod loads,
and shuts it down when the game exits. Your mod does not create or destroy the runtime. You only run scripts.

## Run a script file

Use `runFile` with a resources directory and a flat `.luau` file name. It must run on the main thread.
See [Your first script](../getting-started/chapter-4.md) for a full bootstrap example.

```cpp
auto result = lua::runFile(Mod::get()->getResourcesDir(), "Bootstrap.luau");
if (result.isErr()) {
    log::error("script failed: {}", result.unwrapErr());
}
```

## Run inline source

Use `runScript` when you already have the source as text. You pass a chunk name for logs and errors.

```cpp
auto result = lua::runScript(
    Mod::get()->getResourcesDir(),
    "print('inline')",
    "inline-demo"
);
```

## Run without blocking the main thread

The async variants return a future. They perform the path work on the calling thread,
then hop to the main thread to run the script. Use them when you are not already on the main thread.

```cpp
geode::async::TaskHolder<geode::Result<void>> g_task;

g_task.spawn(
    "luau async bootstrap",
    lua::runFileAsync(Mod::get()->getResourcesDir(), "AsyncBootstrap.luau"),
    [](geode::Result<void> result) {
        if (result.isErr()) {
            log::error("async failed: {}", result.unwrapErr());
        }
    }
);
```

The script still runs on the main thread. Only the wait is asynchronous.

## Check status before running

Read the runtime status before you load scripts. Off the main thread the status reports `NotReady`.

```cpp
if (lua::status() != lua::RuntimeStatus::Ready) {
    log::error("runtime not ready");
    return;
}
log::info("codegen {}", lua::codegenEnabled() ? "on" : "off");
log::info("memory {} / {}", lua::memoryUsage(), lua::memoryLimit());
```

If status is `Panicked`, the runtime hit an unrecoverable Lua panic. Do not call run functions again.

## Read the last error

When a run fails, the result holds an error string. You can also read the last runtime error with `lastError`.
See [Limits and errors](limits-and-errors.md) for the full error-handling pattern.

## Related

- [API reference](api-reference.md)
- [Limits and errors](limits-and-errors.md)

## Source

- `include/LuauAPI.hpp`
- `src/api.cpp`
- `src/main.cpp`
- `mod.json`
