# C++ API reference

## Summary

The public C++ API in `imes::luauapi`. Signatures match `include/LuauAPI.hpp`.

| Call style | Caller thread | Execution |
| --- | --- | --- |
| Sync run and status | Main only | Full work on main |
| Async run | Any (not shutting down) | Prepare on caller, run on main |

See [Getting started](../../getting-started/overview.md) for the user-facing threading rule.

## Integration

Another Geode mod uses LuauAPI like this:

1. Add the LuauAPI mod dependency.
2. Include `include/LuauAPI.hpp` (exported through `api.include` in `mod.json`).
3. Call `runFile` or `runScript` with your resources directory.

Make sure your Geode SDK is up to date.
See [Your first script](../../getting-started/first-script.md) and [Installation](../../getting-started/installation.md) for setup.

Runtime ownership:

- LuauAPI owns the runtime lifecycle.
- Your mod does not create or destroy the runtime.
- See [Architecture](../../contributor/architecture.md).

When a run fails, check these surfaces:

| Error kind | `Result` | `lastError()` |
| --- | --- | --- |
| Sync run failure | Message on `Err` | Updated |
| Async preparation (bad path, oversized file, shutdown) | `Err` on future | Not updated |
| Async execution | `Err` on future | Updated |

See Threading below for per-function thread rules.

## Threading

| API | Caller thread | Notes |
| --- | --- | --- |
| `runFile`, `runScript` | Main only | Full path validation, read, compile, and run |
| `runFileAsync`, `runScriptAsync` | Any (not shutting down) | Path validation and file read on caller, script runs on main |
| `isReady`, `status`, `lastError` | Main only | Off main thread or during shutdown return safe defaults |
| `memoryUsage`, `memoryLimit`, `codegenEnabled` | Main only | Return zeros or false off main thread |

Preparation errors return `Err` on the caller thread for async calls and do not update `lastError()`.
Execution errors populate both the async `Result` and `lastError()`.

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

| Function | Role |
| --- | --- |
| `runFile` | Read and run a `.luau` file. `relativePath` must be a flat `.luau` name inside `resourcesRoot` |
| `runScript` | Run inline source. `chunkName` names the chunk in logs and errors |

Both return `Ok` or `Err` with a message.
See [Limits and errors](limits-and-errors.md) for path and size rules.

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

These prepare on the calling thread, hop to the main thread to run the script, and return a future.
Preparation and execution errors follow the rule in Threading above.
If main-thread dispatch is cancelled, the future resolves with `"luau main-thread execution cancelled"`.

## Status functions

```cpp
bool isReady();
RuntimeStatus status();
std::string lastError();
```

| Function | Role |
| --- | --- |
| `isReady` | True only on the main thread when the runtime is initialized and not shutting down |
| `status` | Runtime status, or `NotReady` off the main thread or while shutting down |
| `lastError` | Copy of the last runtime error string. Empty off the main thread or while shutting down |

## Resource functions

```cpp
std::size_t memoryUsage();
std::size_t memoryLimit();
bool codegenEnabled();
```

| Function | Role |
| --- | --- |
| `memoryUsage` | Current Lua memory use in bytes. Returns `0` off the main thread or while shutting down |
| `memoryLimit` | Memory cap in bytes. Returns `0` off the main thread or while shutting down |
| `codegenEnabled` | True when native code generation is on |

## RuntimeStatus

```cpp
enum class RuntimeStatus {
    NotReady,
    Ready,
    InitFailed,
    Panicked,
};
```

| Value | Meaning |
| --- | --- |
| `NotReady` | Off the main thread, not initialized, or shutting down |
| `Ready` | Runtime is up and scripts can run |
| `InitFailed` | Startup failed |
| `Panicked` | Unrecoverable Lua panic. The runtime will not run scripts again |

## Defaults and caps

`kDefaultScriptDeadlineMs`, memory caps, and script size limits are defined in:

- `include/RuntimeTypes.hpp`
- `src/core/Config.hpp`

See [Limits and errors](limits-and-errors.md).

## Related

- [Getting started](../../getting-started/overview.md)
- [Your first script](../../getting-started/first-script.md)
- [Limits and errors](limits-and-errors.md)
- [Architecture](../../contributor/architecture.md)

## Source

- `include/LuauAPI.hpp`
- `include/RuntimeTypes.hpp`
- `src/api.cpp`
- `mod.json`
