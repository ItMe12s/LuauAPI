# Core concepts

## Summary

This page describes the mental model shared by every part of LuauAPI. Reading it once makes the other pages easier to follow.

## One runtime, one main thread

The game uses a single shared Luau runtime that is created once and reused for every script. Almost every entry point must run on the main thread, and the runtime records the main thread at load time and checks it on each call.

A script entry point called off the main thread returns an error instead of running.

## Runtime lifecycle

The runtime follows the game lifecycle:

- The main thread is recorded at mod load.
- The runtime is created when the mod is loaded.
- The runtime shuts down and cleans up when the game is exiting.

## Resources root and flat paths

Scripts load from a resources directory that the host provides. This directory acts as the root, and scripts and modules cannot escape it.

Script and module names must be flat, which means a single file name with the `.luau` extension. Folders, `..`, and absolute paths are not allowed. This rule is enforced in several places, which keeps all loading inside the root.

## Deadlines

Every script run has a time budget in milliseconds. When a script runs longer than its budget, the runtime interrupts it and raises an error.

The defaults come from `src/lua/Config.hpp`:

- Default script deadline: `250 ms`
- Hook and task callback deadline: `50 ms`

## Memory limit

The runtime uses a custom allocator with a hard cap of `512 MiB`. When an allocation would cross the cap, the allocator refuses it. There is no soft limit.

## Size limits

A single script or module must be `4 MiB` or smaller.

## Bytecode cache

Compiled scripts are cached as bytecode. The cache key includes the file path, size, modify time, and a content hash. The cache holds up to `512` entries and drops the least recently used entry when it is full.

## Codegen

Luau can compile hot functions to native code when the hardware supports it, and the runtime enables this whenever it can. The host can check the current state with `codegenEnabled`.

## Limits and notes

All of the numbers above are defined in `src/lua/Config.hpp`. See [Limits and errors](../cpp/limits-and-errors.md) for the full table.

## Source

- `src/lua/Config.hpp`
- `src/main.cpp`
- `src/api.cpp`
- `src/lua/runtime/Runtime.cpp`
- `include/LuauAPI.hpp`
