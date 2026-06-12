# Runtime

## Summary

The `Runtime` class owns the Lua state and everything around it:

- memory management
- deadline enforcement
- bytecode caching
- bindings setup
- shutdown handling

It lives in `src/core/`.

## One shared instance

The runtime is a single shared instance, accessed through static functions:

- `getOrCreate` returns the instance, creating it if needed.
- `getIfInitialized` returns the instance only if it exists.
- `isInitialized` and `isShuttingDown` report lifecycle state.
- `shutdown` tears the instance down.
- `setMainThreadId` and `isMainThread` handle main thread tracking.

## Construction

The constructor creates the Lua state with a custom bounded allocator, opens the standard libraries,
enables native codegen when the hardware allows, installs callbacks, creates the requirer,
and applies all bindings. After this, `status` reports `Ready`.

## Memory accounting

The state uses `boundedAlloc`, a custom allocator.
It tracks current use in `m_memoryUsage` and caps it at `m_memoryLimit`.
When an allocation would cross the cap, the allocator returns null and Lua reports an out of memory error.

See [Limits and errors](../../reference/cpp/limits-and-errors.md).
The helper logic lives in `src/core/AllocatorAccounting.hpp`.

## Deadlines and budget

`ScriptBudgetGuard` sets a deadline for a run. It nests, so an inner run does not weaken an outer budget.
The guard saves the previous budget and deadline and restores them when it ends.
An interrupt callback checks the wall clock at instruction boundaries and raises an error when the run passes its deadline.
A panic callback handles fatal Lua errors.

## Bytecode cache

`getOrCompileBytecode` compiles source to bytecode or returns a cached copy.
The cache is a least recently used list with an index map.
Before insert, `trimBytecodeCacheForInsert` evicts using one combined check over entry and byte caps plus runtime memory usage.

See [Limits and errors](../../reference/cpp/limits-and-errors.md).

The cache key is built in the module layer. See [Module system](module-system.md).

## Running code

- `runScript` loads and runs source under a budget.
- `protectedCall` wraps `lua_pcall` with a traceback handler and a budget.
  The task and hook layers call it to run Lua callbacks safely.
- Errors are formatted with a traceback and stored in `m_lastError`.

## Resources root

`ResourcesRootScope` sets the current resources root for the length of a run and restores the previous one afterward.
The requirer uses this root to resolve modules.

## Generation counter

`m_generation` increases across runtime restarts.
A `LuaRef` records the generation it was created in.
After a restart, an old reference sees a generation mismatch and reports itself as invalid.

## Shutdown

`registerShutdownHook` adds a cleanup callback.
On shutdown the runtime runs the hooks, releases Lua owned C++ objects, clears field tables, and closes the Lua state.

## Related

- [Architecture](../architecture.md)
- [Bindings framework](bindings-framework.md)
- [Module system](module-system.md)

## Source

- `src/core/Runtime.hpp`
- `src/core/Runtime.cpp`
- `src/core/AllocatorAccounting.hpp`
- `src/core/Config.hpp`
