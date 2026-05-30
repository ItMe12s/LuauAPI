# Core concepts

## Summary

This page describes the mental model shared by every part of LuauAPI.

## One runtime, one main thread

The game uses a single shared Luau runtime that is created once and reused for every script.
Almost every entry point must run on the main thread, and the runtime records the main thread at load time and checks it on each call.

A script entry point called off the main thread returns an error instead of running.

## Runtime lifecycle

The runtime follows the game lifecycle:

- The main thread is recorded at mod load.
- The runtime is created when the mod is loaded.
- The runtime shuts down and cleans up when the game is exiting.

## Resources root and flat paths

Scripts load from a resources directory that the host provides. This directory acts as the root, and scripts and modules cannot escape it.

Script and module names must be flat, which means a single file name with the `.luau` extension. Folders, `..`, and absolute paths are not allowed.
This rule is enforced in several places, which keeps all loading inside the root.

## Deadlines and memory

Every script run has a time budget in milliseconds. When a script runs longer than its budget, the runtime interrupts it and raises an error.
Hook and task callbacks use a shorter budget than a normal script run.

The runtime uses a custom allocator with a hard memory cap. When an allocation would cross the cap, the allocator refuses it.
There is no soft limit.

A single script or module must stay within the size limit. Compiled scripts are cached as bytecode with a least-recently-used eviction policy.

See [Limits and errors](../cpp/limits-and-errors.md) for the full table of values.

## Native code generation (Luau JIT)

Luau can compile hot functions to native code when the hardware supports it, and the runtime enables this whenever it can
 The host can check the current state with `codegenEnabled`.

## Source

- `src/lua/Config.hpp`
- `src/main.cpp`
- `src/api.cpp`
- `src/lua/runtime/Runtime.cpp`
- `include/LuauAPI.hpp`
