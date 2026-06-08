# C++ API reference

## Summary

This page lists the public C++ API in `imes::luauapi`. The signatures match `include/LuauAPI.hpp`.

## Run functions

### runFile

```cpp
geode::Result<void> runFile(
    std::filesystem::path const& resourcesRoot,
    std::filesystem::path const& relativePath,
    int deadlineMs = 250
);
```

Reads and runs a `.luau` file, and must run on the main thread.
`relativePath` must be a flat `.luau` name inside `resourcesRoot`.
It cannot be absolute or contain folders, and must be within the size limit.
It returns `Ok` on success, or `Err` with a message.

### runScript

```cpp
geode::Result<void> runScript(
    std::filesystem::path const& resourcesRoot,
    std::string_view source,
    std::string_view chunkName,
    int deadlineMs = 250
);
```

Runs inline source, and must run on the main thread. `chunkName` names the chunk in logs and errors.
The source must be within the size limit. It returns `Ok` or `Err`.

### runFileAsync

```cpp
arc::Future<geode::Result<void>> runFileAsync(
    std::filesystem::path resourcesRoot,
    std::filesystem::path relativePath,
    int deadlineMs = 250
);
```

Behaves like `runFile`, but returns a future.
It resolves the path and reads the file on the calling thread, then runs the script on the main thread.
If the main thread work is cancelled, it returns an error.

### runScriptAsync

```cpp
arc::Future<geode::Result<void>> runScriptAsync(
    std::filesystem::path resourcesRoot,
    std::string source,
    std::string chunkName,
    int deadlineMs = 250
);
```

Behaves like `runScript`, but returns a future and runs the script on the main thread.

## Status functions

### isReady

```cpp
bool isReady();
```

Returns true only when called on the main thread, the runtime is initialized, and the runtime is not shutting down.

### status

```cpp
RuntimeStatus status();
```

Returns the runtime status. Off the main thread or while shutting down, it returns `NotReady`.

### lastError

```cpp
std::string lastError();
```

Returns a copy of the last runtime error string. It is empty off the main thread or while shutting down.

## Resource functions

### memoryUsage and memoryLimit

```cpp
std::size_t memoryUsage();
std::size_t memoryLimit();
```

Return the current Lua memory use and the cap in bytes.
Both return `0` off the main thread or while shutting down.
See [Limits and errors](limits-and-errors.md) for the cap value.

### codegenEnabled

```cpp
bool codegenEnabled();
```

Returns true when native code generation is on.

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
- `Panicked`: the runtime hit an unrecoverable Lua panic and will not run scripts again.

## Default deadline

```cpp
inline constexpr int kDefaultScriptDeadlineMs = 250;
```

The run functions use this default when you do not pass `deadlineMs`.

## Source

- `include/LuauAPI.hpp`
- `src/api.cpp`
