# Reference: task

## Summary

This page is the type reference for the `task` library. For usage guidance, see [Tasks and time](../guide/tasks-and-time.md). The types match the stub `tools/luau_codegen/extra_bindings/task.dluau`.

## Types

```lua
type TaskHandle = {
    cancel: (self: TaskHandle) -> (),
}

type TaskNamespace = {
    spawn: (fn: (...any) -> ...any, ...any) -> (),
    delay: (seconds: number, fn: () -> ()) -> TaskHandle,
    every: (seconds: number, fn: () -> ()) -> TaskHandle,
    defer: (fn: () -> ()) -> TaskHandle,
    cancel: (handle: TaskHandle) -> (),
}
```

## Functions

### task.spawn

```lua
task.spawn(fn: (...any) -> ...any, ...any) -> ()
```

Runs `fn` immediately under the callback budget. Extra arguments are passed to `fn`. Errors are logged rather than thrown, and the call returns nothing.

### task.delay

```lua
task.delay(seconds: number, fn: () -> ()) -> TaskHandle
```

Runs `fn` once after `seconds` and returns a handle. A negative value is clamped to zero.

### task.every

```lua
task.every(seconds: number, fn: () -> ()) -> TaskHandle
```

Runs `fn` repeatedly every `seconds` and returns a handle. The interval must be greater than zero, otherwise the call raises an error.

### task.defer

```lua
task.defer(fn: () -> ()) -> TaskHandle
```

Runs `fn` once on the next tick and returns a handle.

### task.cancel

```lua
task.cancel(handle: TaskHandle) -> ()
```

Cancels a scheduled task. You can also call `handle:cancel()`.

## Limits

- The most scheduled tasks at once is `4096`. Going over raises an error.
- Each callback runs with a `50 ms` budget.

## Source

- `tools/luau_codegen/extra_bindings/task.dluau`
- `src/lua/bindings/task/TaskBinding.cpp`
- `src/lua/Config.hpp`
