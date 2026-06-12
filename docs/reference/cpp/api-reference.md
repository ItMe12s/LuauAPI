# C++ API reference

## Summary

The public C++ API in `imes::luauapi`. Signatures match `include/LuauAPI.hpp`.
All functions must be called on the main thread.

## Run functions

```cpp
geode::Result<void> runFile(
    std::filesystem::path const& resourcesRoot,
    std::filesystem::path const& relativePath,
    int deadlineMs = kDefaultScriptDeadlineMs
);

geode::Result<void> runScript(
    std::filesystem::path const& resourcesRoot,
    std::string_view source,
    std::string_view chunkName,
    int deadlineMs = kDefaultScriptDeadlineMs
);
```

`runFile` reads and runs a `.luau` file.
`relativePath` must be a flat `.luau` name inside `resourcesRoot`.
See [Limits and errors](limits-and-errors.md) for path and size rules.
`runScript` runs inline source. `chunkName` names the chunk in logs and errors.
Both return `Ok` or `Err` with a message.

## Async run functions

```cpp
arc::Future<geode::Result<void>> runFileAsync(
    std::filesystem::path resourcesRoot,
    std::filesystem::path relativePath,
    int deadlineMs = kDefaultScriptDeadlineMs
);

arc::Future<geode::Result<void>> runScriptAsync(
    std::filesystem::path resourcesRoot,
    std::string source,
    std::string chunkName,
    int deadlineMs = kDefaultScriptDeadlineMs
);
```

These read on the calling thread, run the script on the main thread, and return a future.

## Status functions

```cpp
bool isReady();
RuntimeStatus status();
std::string lastError();
```

`isReady` is true only on the main thread when the runtime is initialized and not shutting down.
`status` returns the runtime status, or `NotReady` off the main thread or while shutting down.
`lastError` returns a copy of the last runtime error string, empty off the main thread or while shutting down.

## Resource functions

```cpp
std::size_t memoryUsage();
std::size_t memoryLimit();
bool codegenEnabled();
```

`memoryUsage` and `memoryLimit` return current Lua memory use and the cap in bytes.
Both return `0` off the main thread or while shutting down.
`codegenEnabled` is true when native code generation is on.

## RuntimeStatus

```cpp
enum class RuntimeStatus {
    NotReady,
    Ready,
    InitFailed,
    Panicked,
};
```

- `NotReady`: off the main thread, not initialized, or shutting down.
- `Ready`: runtime is up and scripts can run.
- `InitFailed`: startup failed.
- `Panicked`: an unrecoverable Lua panic. The runtime will not run scripts again.

## Defaults and caps

`kDefaultScriptDeadlineMs`, memory caps, and script size limits are defined in `include/RuntimeTypes.hpp` and `src/core/Config.hpp`.

See [Limits and errors](limits-and-errors.md).

## Related

- [Integration guide](integration-guide.md)
- [Your first script](../../getting-started/first-script.md)

## Source

- `include/LuauAPI.hpp`
- `include/RuntimeTypes.hpp`
- `src/api.cpp`
