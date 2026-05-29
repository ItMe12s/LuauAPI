# Tasks and time

## Summary

The `task` library schedules callbacks, and the `time` library reads clocks. Tasks run on the game tick and follow the game, so they freeze when the game pauses.

## task functions

### task.spawn

Runs a function immediately, under the script budget. Extra arguments are passed to the function. Errors are logged rather than thrown, and the call returns nothing.

```lua
task.spawn(function(name)
    print("hello " .. name)
end, "world")
```

### task.delay

Runs a function once after a delay in seconds and returns a handle. A negative delay is treated as zero.

```lua
task.delay(1.5, function()
    print("1.5 seconds later")
end)
```

### task.every

Runs a function repeatedly on a fixed interval in seconds and returns a handle. The interval must be greater than zero, otherwise the call raises an error.

```lua
local handle = task.every(0.5, function()
    print("tick")
end)
```

### task.defer

Runs a function once on the next tick and returns a handle.

```lua
task.defer(function()
    print("next frame")
end)
```

### task.cancel

Cancels a scheduled task by its handle.

```lua
task.cancel(handle)
```

## The handle

`task.delay`, `task.every`, and `task.defer` each return a handle. The handle has a `cancel` method, so you can cancel either with the method or with `task.cancel`.

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

## time functions

- `time.now()` returns the seconds since the runtime loaded the task library. It uses a steady clock.
- `time.unix()` returns the seconds since the unix epoch. It uses the system clock.

```lua
print(time.now(), time.unix())
```

## How tasks run

Tasks are driven by the game scheduler. They advance each frame by the frame delta, follow the game tick, and freeze when the game pauses. Each callback runs on the main thread with a `50 ms` budget.

## Limits and notes

- The most you can schedule at once is `4096`. Going over raises an error.
- `task.every` requires an interval greater than zero.
- Callback errors are logged rather than thrown.

## Related

- [task reference](../reference/task.md)
- [time reference](../reference/time.md)

## Source

- `src/lua/bindings/task/TaskBinding.cpp`
- `src/lua/bindings/task/TaskScheduler.cpp`
- `src/lua/Config.hpp`
