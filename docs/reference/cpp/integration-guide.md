# C++ integration guide

## Summary

How another Geode mod uses LuauAPI to run scripts from C++.
You depend on the mod, include the header, and call the sync run functions.
See [Getting started](../../getting-started/overview.md) for the main-thread rule
and [C++ API reference](api-reference.md) Threading for per-function thread rules.
Async variants may be called from a worker thread when the runtime is not shutting down.

## Before you start

Set up the dependency, resources directory, header, and your first `runFile` call.
See [Your first script](../../getting-started/first-script.md) and [Installation](../../getting-started/installation.md).

LuauAPI exports its public header through `api.include` in `mod.json`. Make sure your Geode SDK is up to date.

## Runtime ownership

LuauAPI owns the runtime lifecycle.
Your mod does not create or destroy the runtime.
See [Architecture](../../contributor/architecture.md).

## Run scripts

Call `runFile` or `runScript` with your resources directory.
See [C++ API reference](api-reference.md) Run functions and Threading.

Async variants return `arc::Future<geode::Result<void>>`.
See [C++ API reference](api-reference.md) Async run functions.

## Status and errors

See [C++ API reference](api-reference.md) Status functions before running scripts.
When a sync run fails, read the message from `Result` and optionally `lastError()`.

Async preparation errors (bad path, file too large) appear only in the future `Result`.
Execution errors appear in both the future `Result` and `lastError()`.
See [C++ API reference](api-reference.md) Threading for where each error is reported.

See [Limits and errors](limits-and-errors.md) for caps and error strings.

## Related

- [Getting started](../../getting-started/overview.md)
- [Your first script](../../getting-started/first-script.md)
- [C++ API reference](api-reference.md)
- [Limits and errors](limits-and-errors.md)
- [Architecture](../../contributor/architecture.md)

## Source

- `include/LuauAPI.hpp`
- `src/api.cpp`
- `src/main.cpp`
- `mod.json`
