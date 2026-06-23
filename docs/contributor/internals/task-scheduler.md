# Task scheduler

## Summary

The task scheduler runs `task` callbacks on the game tick. It lives in `src/bindings/task/`.
The binding exposes the `task` and `time` libraries,
and the scheduler stores the callbacks and fires them when due.

## The binding

`TaskBinding.cpp` builds the `task` and `time` global tables and registers them.
It sets the time origin, registers the handle metatable, arms the tick,
and adds a shutdown hook that disarms the tick and clears the scheduler.
Each scheduled call stores its Lua function as a `LuaRef`.
The handle returned to Lua is a small userdata that holds a task id and a `cancel` method.

## The scheduler

`TaskScheduler` is a single instance.

Each task contains:

- an id
- a callback reference
- the time remaining until it next fires
- an interval
- a cancelled flag

The interval encodes the kind of task:

- `0` means one shot, used by `task.delay`.
- A value greater than zero means repeating, used by `task.every`.
- A value less than zero means deferred to the next tick, used by `task.defer`.

`task.spawn` does not schedule anything. It runs the callback immediately under the callback budget.

## Advancing

`advance(dt, L)` runs each frame.
It also calls `diag::flushIfNeeded` so the crash sidecar file stays warm. See [Crash sidecar](crash-sidecar.md).
It lowers each task timer by the frame delta and fires the tasks that are due.
To fire a task it pushes the callback, sets the resources root scope,
and calls the runtime protected call with the callback budget.
A repeating task is rescheduled, while a one shot or deferred task is marked cancelled.
Cancelled tasks are then removed.

## Game integration

`armTaskTick` creates a small `CCNode` and schedules its update with the Cocos2d scheduler.
The update calls `advance`. Tasks use frame delta, so speedhacks change their timing.
Tasks are not paused when the game pauses (game as in playing a level and the pause menu).
If the director or scheduler is not ready, `armTaskTick` queues a one-shot retry on the main thread until arming works,
so early scheduled callbacks still run. Failures are logged once. `disarmTaskTick` removes the node and stops any pending arm retry.

## Limits

Task count and callback budget caps are in [Limits and errors](../../reference/cpp/limits-and-errors.md).
`task.every` rejects an interval that is not greater than zero.

See [Limits and errors](../../reference/cpp/limits-and-errors.md).

## Related

- [Architecture](../architecture.md)
- [tasks and time](../../reference/lua/tasks.md)
- [ImGui draw scheduler](imgui-draw-scheduler.md)
- [Crash sidecar](crash-sidecar.md)
- [Runtime](runtime.md)
- [Limits and errors](../../reference/cpp/limits-and-errors.md)

## Source

- `src/bindings/task/TaskBinding.cpp`
- `src/bindings/task/TaskScheduler.hpp`
- `src/bindings/task/TaskScheduler.cpp`
- `src/core/Config.hpp`
