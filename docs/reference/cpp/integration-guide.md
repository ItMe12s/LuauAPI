# C++ integration guide

## Summary

How another Geode mod uses LuauAPI to run scripts from C++.
You depend on the mod, include the header, and call the run functions on the main thread.

## Before you start

Set up the dependency and header first. See [Your first script](../../getting-started/first-script.md)
for the `mod.json` dependency, resources, and include.

The examples below use the alias:

```cpp
namespace lua = imes::luauapi;
```

## You do not start the runtime

The LuauAPI mod owns the runtime lifecycle.
It records the main thread at load, creates the runtime when its mod loads, and shuts it down when the game exits.
Your mod does not create or destroy the runtime. You only run scripts.

## Run a script file

Use `runFile` with a resources directory and a flat `.luau` file name, on the main thread.

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

The async variants return a future.
They do the path work on the calling thread, then hop to the main thread to run the script.
Use them when you are not already on the main thread.

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
Both async functions return `arc::Future<geode::Result<void>>`,
and the `TaskHolder` above is the normal Geode pattern for consuming that future.

## Check status before running

Read the runtime status before you load scripts. Off the main thread it reports `NotReady`.
If it is `Panicked`, the runtime hit an unrecoverable Lua panic, so do not call run functions again.

```cpp
if (lua::status() != lua::RuntimeStatus::Ready) {
    log::error("runtime not ready");
    return;
}
log::info("codegen {}", lua::codegenEnabled() ? "on" : "off");
log::info("memory {} / {}", lua::memoryUsage(), lua::memoryLimit());
```

## Read the last error

When a run fails, the result holds an error string. You can also read the last runtime error with `lastError`.
See [Limits and errors](limits-and-errors.md) for the full error-handling pattern.

## Related

- [API reference](api-reference.md)
- [Limits and errors](limits-and-errors.md)
- [Your first script](../../getting-started/first-script.md)

## Source

- `include/LuauAPI.hpp`
- `src/api.cpp`
- `src/main.cpp`
- `mod.json`
