# tasks and time

## Summary

The `task` library schedules callbacks and the `time` library reads clocks.
Tasks run on the game tick, so they freeze when the game pauses.
Types match `tools/luau_codegen/extra_bindings/task.dluau`.

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
Errors are logged rather than thrown, and the call returns nothing.

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

```lua
local ticks = 0
local handle
handle = task.every(0.5, function()
    ticks += 1
    if ticks >= 5 then
        handle:cancel() -- same as task.cancel(handle)
    end
end)
```

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

Tasks are driven by the game scheduler and advance each frame by the frame delta.
Each callback runs on the main thread under the callback budget.
Scheduling has a maximum task count, and going over raises an error.
Callback errors are logged rather than thrown.
One-shot tasks end after they run, repeated tasks stop after an error.

See [Limits and errors](../cpp/limits-and-errors.md) for caps and error strings.

## Related

- [Globals](globals.md)
- [Sharing APIs between mods](sharing-apis.md)
- [Limits and errors](../cpp/limits-and-errors.md)

## Source

- `tools/luau_codegen/extra_bindings/task.dluau`
- `src/bindings/task/TaskBinding.cpp`
- `src/bindings/task/TaskScheduler.cpp`
- `src/core/Config.hpp`
