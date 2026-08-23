# Task scheduler

## Summary

The task scheduler runs `task` callbacks on the game tick. It lives in `src/bindings/task/`.
The binding exposes `task` and `time`. The scheduler stores entries and fires them when due.

## The binding

`TaskBinding.cpp` does the following:

- Builds the `task` and `time` globals
- Sets the time origin
- Registers the handle metatable
- Arms the tick
- Hooks shutdown to disarm and clear

Each entry is a `LuaRef` to a function or waiting thread.
Handles are userdata with a task id and `cancel`.

`task.wait` refs the current thread, schedules a one-shot wait, and yields.
When due, the scheduler resumes it with elapsed seconds.

## The scheduler

`TaskScheduler` is a single instance. Each task holds:

- id
- callback (function or suspended thread)
- remaining time
- interval
- elapsed since schedule
- cancelled flag
- `isThread` for wait resumes

Interval meaning:

- `0` means one shot (`task.delay`, `task.wait`)
- greater than zero means repeating (`task.every`)
- deferred tasks use a separate store (`task.defer`)

`task.spawn` does not schedule. It resumes a fresh coroutine under the callback budget immediately.

## Coroutine execution

`task.spawn` uses `LuaCallback::fireStackOnThread`.
`task.delay`, `task.every`, and `task.defer` use `LuaCallback::fireOnThread`.
Both paths funnel into `fireStackOnThread`. `fireOnThread` is the wrapper for a stored `LuaRef` with no arguments.
The helper creates a `lua_newthread`, moves the function, and `lua_resume`s under the budget.
`LUA_OK` and `LUA_YIELD` count as success.
`task.wait` uses `LuaCallback::resumeThread` with elapsed seconds.
No diagnostics boundary frames on the resume path.

Script cancel, overlap, and budget rules: [tasks and time](../../reference/lua/tasks.md).

## Advancing

The tick node calls `advance(dt, L)` each frame.
`advance` updates elapsed and remaining, then fires due tasks.
Functions get a fresh coroutine. Waits resume their thread with elapsed seconds.
Repeating tasks reschedule. One shot, deferred, and wait tasks cancel after fire.
Then the tick polls Geode task handles and calls `diag::flushIfNeeded`.
See [Crash sidecar](crash-sidecar.md).

## Game integration

`armTaskTick` schedules a small `CCNode` update on the Cocos2d scheduler.
Tasks use frame delta, so speedhacks affect timing.
They are not paused with the game pause menu.
The tick node also records the main thread id if `$on_mod(Loaded)` has not set it yet.
If the director or scheduler is not ready, arming retries on the main thread until it works.
Failures log once. `disarmTaskTick` removes the node and stops pending retries.

## Limits

Caps are in [Limits and errors](../../reference/cpp/limits-and-errors.md).
`task.every` rejects a non-positive interval.
`task.wait` rejects a non-yieldable call.

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
- `src/framework/schedule/ScheduledHandleBinding.hpp`
- `src/framework/schedule/CancellableSlots.hpp`
- `src/bindings/geode/GeodeTaskHandleBinding.cpp`
- `src/framework/callback/LuaCallback.hpp`
- `src/core/Config.hpp`
