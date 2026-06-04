# Runtime

## Summary

The `Runtime` class owns the Lua state and everything around it, including memory, deadlines, the bytecode cache, bindings setup, and shutdown.
It lives in `src/lua/runtime/`.

## One shared instance

The runtime is a single shared instance, accessed through static functions:

- `getOrCreate` returns the instance, creating it if needed.
- `getIfInitialized` returns the instance only if it exists.
- `isInitialized` and `isShuttingDown` report lifecycle state.
- `shutdown` tears the instance down.
- `setMainThreadId` and `isMainThread` handle main thread tracking.

## Construction

The constructor creates the Lua state with a custom bounded allocator, opens the standard libraries,
enables native codegen when the hardware allows, installs callbacks, creates the requirer, and applies all bindings.
After this, `status` reports `Ready`.

## Memory accounting

The state uses `boundedAlloc`, a custom allocator.
It tracks current use in `m_memoryUsage` and caps it at `m_memoryLimit`, which starts at `512 MiB`.
When an allocation would cross the cap, the allocator returns null and Lua reports an out of memory error.
The helper logic lives in `src/lua/runtime/AllocatorAccounting.hpp`.

## Deadlines and budget

`ScriptBudgetGuard` sets a deadline for a run. It nests, so an inner run does not weaken an outer budget.
The guard saves the previous budget and deadline and restores them when it ends.

An interrupt callback checks the wall clock at instruction boundaries. When the run passes its deadline, it raises an error.
A panic callback handles fatal Lua errors.

## Bytecode cache

`getOrCompileBytecode` compiles source to bytecode or returns a cached copy.
The cache is a least recently used list with an index map, and it holds up to `512` entries.
Before insert, `trimBytecodeCacheForInsert` evicts using one combined check over cache byte/entry limits and runtime memory usage (`bytecodeCacheInsertNeedsEviction` in `BytecodeCacheAccounting.hpp`).
The cache key is built in the module layer from the path, size, modify time, and content hash.
See [Module system](module-system.md).

## Running code

- `runScript` loads and runs source under a budget.
- `protectedCall` wraps `lua_pcall` with a traceback handler and a budget. The task and hook layers call it to run Lua callbacks safely.
- Errors are formatted with a traceback and stored in `m_lastError`.

## Resources root

`ResourcesRootScope` sets the current resources root for the length of a run and restores the previous one afterward. The requirer uses this root to resolve modules.

## Generation counter

`m_generation` increases across runtime restarts. A `LuaRef` records the generation it was created in. After a restart, an old reference sees a generation mismatch and reports itself as invalid.

## Shutdown

`registerShutdownHook` adds a cleanup callback. On shutdown the runtime runs the hooks, releases Lua owned C++ objects, clears field tables, and closes the Lua state.

## Source

- `src/lua/runtime/Runtime.hpp`
- `src/lua/runtime/Runtime.cpp`
- `src/lua/runtime/AllocatorAccounting.hpp`
- `src/lua/Config.hpp`
