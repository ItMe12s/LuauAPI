# Task scheduler

## Summary

The task scheduler runs `task` callbacks on the game tick. It lives in `src/lua/bindings/task/`. The binding exposes the `task` and `time` libraries, and the scheduler stores the callbacks and fires them when due.

## The binding

`TaskBinding.cpp` builds the `task` and `time` global tables and registers them with a binding. It also sets the time origin, registers the handle metatable, arms the tick, and adds a shutdown hook that disarms the tick and clears the scheduler.

Each scheduled call stores its Lua function as a `LuaRef`. The handle returned to Lua is a small userdata that holds a task id and provides a `cancel` method.

## The scheduler

`TaskScheduler` is a single instance. Each task holds:

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

`advance(dt, L)` runs each frame. It lowers each task timer by the frame delta and fires the tasks that are due. To fire a task, it pushes the callback, sets the resources root scope, and calls the runtime protected call with the callback budget. A repeating task is rescheduled, while a one shot or deferred task is marked cancelled. Cancelled tasks are then removed.

## Game integration

`armTaskTick` creates a small `CCNode` and schedules its update with the Cocos2d scheduler. The update calls `advance`. Because it uses the game scheduler, tasks follow the game tick and freeze when the game pauses. `disarmTaskTick` unschedules and releases the node.

## Limits

The scheduler holds at most `4096` tasks. The binding checks capacity before adding and raises an error when full. `task.every` rejects an interval that is not greater than zero.

## Source

- `src/lua/bindings/task/TaskBinding.cpp`
- `src/lua/bindings/task/TaskScheduler.hpp`
- `src/lua/bindings/task/TaskScheduler.cpp`
- `src/lua/Config.hpp`
