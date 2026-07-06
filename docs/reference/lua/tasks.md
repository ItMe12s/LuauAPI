# tasks and time

## Summary

The `task` library schedules callbacks and the `time` library reads clocks.
Tasks run on the game tick. They keep running when the game pauses. Speedhacks change task timing because timers use frame delta.
Types match `tools/luau_codegen/extra_bindings/task.dluau`.

`loadstring` is a global, not part of `task`.
See [globals](globals.md).

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

type TimeNamespace = {
    now: () -> number,
    unix: () -> number,
}
```

## task.spawn

```lua
task.spawn(fn: (...any) -> ...any, ...any) -> ()
```

Runs `fn` immediately under the callback budget. Extra arguments are passed to `fn`.
Raises `task.spawn requires an initialized runtime` when the runtime is not ready.
Errors inside `fn` are logged. The call returns nothing.

```lua
task.spawn(function(name)
    print("hello " .. name)
end, "world")
```

## task.delay

```lua
task.delay(seconds: number, fn: () -> ()) -> TaskHandle
```

Runs `fn` once after `seconds` and returns a handle. A negative value is clamped to zero.

## task.every

```lua
task.every(seconds: number, fn: () -> ()) -> TaskHandle
```

Runs `fn` repeatedly every `seconds` and returns a handle.
The interval must be greater than zero, otherwise the call raises an error.

## task.defer

```lua
task.defer(fn: () -> ()) -> TaskHandle
```

Runs `fn` once on the next tick and returns a handle.

## task.cancel

```lua
task.cancel(handle: TaskHandle) -> ()
```

Cancels a scheduled task. You can also call `handle:cancel()`.

## The handle

Keep the handle while you expect the callback to run.
Dropping it cancels the task when Lua collects the handle userdata.

See [Examples](../../getting-started/examples.md).

## time.now and time.unix

```lua
time.now() -> number
time.unix() -> number
```

`time.now` returns the seconds since the runtime loaded the task library,
using a steady clock, so it suits measuring elapsed time.
`time.unix` returns the seconds since the unix epoch, using the system clock, so it suits wall clock time.

```lua
print(time.now(), time.unix())
```

## Limits

Tasks use the game scheduler and share the main-thread callback budget.
See [Getting started](../../getting-started/overview.md) and [Limits and errors](../cpp/limits-and-errors.md).

## Related

- [Getting started](../../getting-started/overview.md)
- [Examples](../../getting-started/examples.md)
- [Task scheduler](../../contributor/internals/task-scheduler.md)
- [globals](globals.md)
- [sharing APIs between mods](sharing-apis.md)
- [Limits and errors](../cpp/limits-and-errors.md)

## Source

- `tools/luau_codegen/extra_bindings/task.dluau`
- `src/bindings/task/TaskBinding.cpp`
- `src/bindings/task/TaskScheduler.cpp`
- `src/core/Config.hpp`
