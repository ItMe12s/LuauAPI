# tasks and time

## Summary

The `task` library schedules callbacks and the `time` library reads clocks.
Scheduled tasks run on the game tick. They keep running when the game pauses.
Speedhacks change task timing because timers use frame delta.
`task.spawn`, `task.delay`, `task.every`, and `task.defer` run on fresh coroutines, so `task.wait` works inside them.

`loadstring` is a global, not part of `task`.
See [globals](globals.md).

## task.spawn

```lua
task.spawn(fn: (...any) -> ...any, ...any) -> ()
```

Runs `fn` now under the callback budget. Extra arguments go to `fn`.
Raises `task.spawn requires an initialized runtime` when the runtime is not ready.
Errors inside `fn` are logged. Returns nothing.

```lua
task.spawn(function(name)
    print("hello " .. name)
end, "world")
```

## task.delay

```lua
task.delay(seconds: number, fn: () -> ()) -> TaskHandle
```

Runs `fn` once after `seconds` and returns a handle. Negative or NaN clamps to zero.

```lua
local handle = task.delay(5, function()
    print("five seconds passed")
end)

-- Changed your mind.
handle:cancel()
```

## task.every

```lua
task.every(seconds: number, fn: () -> ()) -> TaskHandle
```

Runs `fn` every `seconds` and returns a handle.
Interval must be greater than zero or the call raises `task.every: interval must be > 0`.
If a callback waits longer than the interval, later runs can overlap.

## task.defer

```lua
task.defer(fn: () -> ()) -> TaskHandle
```

Runs `fn` once on the next tick and returns a handle.

## task.wait

```lua
task.wait(seconds: number?) -> number
```

Yields for `seconds` (default `0`) and returns actual elapsed seconds.
Negative or NaN clamps to zero. `task.wait(0)` waits until the next tick.
Must run from a coroutine or task callback.
Otherwise raises `task.wait must be called from a coroutine or task callback`.

```lua
task.spawn(function()
    local elapsed = task.wait(0.5)
    print("waited", elapsed)
end)
```

## task.cancel

```lua
task.cancel(handle: TaskHandle) -> ()
```

Cancels a scheduled task. Same as `handle:cancel()`.
Stops future runs. An in-flight `task.wait` still finishes.

## The handle

Keep the handle while you expect the callback to run.
Dropping it cancels the task when Lua collects the handle userdata.

Use a self-referencing handle to stop a repeating task from inside itself:

```lua
local ticks = 0
local everyHandle
everyHandle = task.every(0.5, function()
    ticks += 1
    print("tick", ticks)
    if ticks >= 5 then
        everyHandle:cancel()
    end
end)
```

## time.now and time.unix

```lua
time.now() -> number
time.unix() -> number
```

`time.now` is seconds since the task library loaded (steady clock).
`time.unix` is seconds since the unix epoch (system clock).

```lua
print(time.now(), time.unix())
```

## Limits

Tasks use the game scheduler and share the main-thread callback budget.
Each resume gets a fresh budget.
See [Getting started](../../getting-started/overview.md) and [Limits and errors](../cpp/limits-and-errors.md).

## Related

- [globals](globals.md)
- [sharing APIs between mods](sharing-apis.md)
- [Getting started](../../getting-started/overview.md)
- [Limits and errors](../cpp/limits-and-errors.md)
- [Task scheduler](../../contributor/internals/task-scheduler.md)

## Source

- `tools/luau_codegen/extra_bindings/task.dluau`
- `src/bindings/task/TaskBinding.cpp`
- `src/bindings/task/TaskScheduler.cpp`
- `src/core/Config.hpp`
