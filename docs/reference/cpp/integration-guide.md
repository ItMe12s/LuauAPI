# C++ integration guide

## Summary

How another Geode mod uses LuauAPI to run scripts from C++.
You depend on the mod, include the header, and call the sync run functions on the main thread.
Async variants may be called from a worker thread.

## Before you start

Set up the dependency, resources directory, header, and your first `runFile` call.
See [Your first script](../../getting-started/first-script.md) and [Installation](../../getting-started/installation.md).

## Runtime ownership

LuauAPI owns the runtime lifecycle.
Your mod does not create or destroy the runtime.
See [Architecture Lifecycle](../../contributor/architecture.md).

## Run scripts

Call `runFile` or `runScript` on the main thread with your resources directory.
See [API reference](api-reference.md) for signatures.

Async variants validate paths and read files on the calling thread, then run on the main thread.
They return `arc::Future<geode::Result<void>>`.
You can call them from a worker thread when the runtime is not shutting down.

## Status and errors

Check `status()` on the main thread before running scripts.
When a sync run fails, read the message from `Result` and optionally `lastError()`.

Async preparation errors (bad path, file too large) appear only in the future `Result`.
Execution errors appear in both the future `Result` and `lastError()`.

See [Limits and errors](limits-and-errors.md) How errors reach you.

## Related

- [API reference](api-reference.md)
- [Your first script](../../getting-started/first-script.md)

## Source

- `include/LuauAPI.hpp`
- `src/api.cpp`
- `src/main.cpp`
- `mod.json`
