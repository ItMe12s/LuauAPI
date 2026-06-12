# C++ integration guide

## Summary

How another Geode mod uses LuauAPI to run scripts from C++.
You depend on the mod, include the header, and call the run functions on the main thread.

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

Async variants read files off thread, then run on the main thread.
They return `arc::Future<geode::Result<void>>`.

## Status and errors

Check `status()` before running scripts.
When a run fails, read the message from `Result` and optionally `lastError()`.

See [Limits and errors](limits-and-errors.md) How errors reach you.

## Related

- [API reference](api-reference.md)
- [Your first script](../../getting-started/first-script.md)

## Source

- `include/LuauAPI.hpp`
- `src/api.cpp`
- `src/main.cpp`
- `mod.json`
