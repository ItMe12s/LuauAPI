# Chapter 5: Core concepts

## Summary

This page describes the mental model shared by every part of LuauAPI.

## One runtime, one main thread

The game uses one shared Luau runtime. It is created once and reused for every script.
Almost every entry point must run on the main thread.
The runtime records the main thread at load time and checks it on each call.

A script entry point called off the main thread returns an error instead of running.

## Runtime lifecycle

The runtime follows the game lifecycle:

- The main thread is recorded at mod load.
- The runtime is created when the mod loads.
- The runtime shuts down and cleans up when the game exits.

## Resources root and flat paths

Scripts load from a resources directory that the host provides.
This directory is the root, and scripts and modules cannot escape it.

Script and module names must be flat. That means a single file name with the `.luau` extension.
Folders, `..`, and absolute paths are not allowed.
Several places enforce this rule, which keeps all loading inside the root.

## Deadlines and memory

Every script run has a time budget in milliseconds.
When a script runs longer than its budget, the runtime interrupts it and raises an error.
Hook and task callbacks use a shorter budget than a normal script run.

The runtime uses a custom allocator with a hard memory cap.
When an allocation would cross the cap, the allocator refuses it.
There is no soft limit.

A single script or module must stay within the size limit.
Compiled scripts are cached as bytecode with a least-recently-used eviction policy.

See [Limits and errors](../cpp/limits-and-errors.md) for the full table of values.

## Native code generation (Luau JIT)

Luau can compile hot functions to native code when the hardware supports it. The runtime turns this on whenever it can.
The host can check the current state with `codegenEnabled`.

## Where to go next

- Script authors: [Overview](../lua/overview.md)
- C++ integrators: [Integration guide](../cpp/integration-guide.md)

Back to [Chapter 4: Your first script](chapter-4.md).

## Source

- `src/lua/Config.hpp`
- `src/main.cpp`
- `src/api.cpp`
- `src/lua/runtime/Runtime.cpp`
- `include/LuauAPI.hpp`
