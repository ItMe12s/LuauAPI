-- `task` scheduler & `time` clock.
-- Timers use CCScheduler, follow the game's tick, freeze on pause, and use seconds.

print(`task demo starting at t={time.now()}s (unix {time.unix()})`)

-- Run once after 1.5 seconds.
task.delay(1.5, function()
    print(`delayed hello at t={time.now()}s`)
end)

-- Run right now under the script budget (errors are logged, not thrown).
task.spawn(function(name)
    print(`spawned now for {name}`)
end, "world")

-- Run once on the next tick.
task.defer(function()
    print("deferred to next frame")
end)

-- Run every 0.5s. Keep the handle so we can cancel it.
local ticks = 0
local handle
handle = task.every(0.5, function()
    ticks += 1
    print(`tick {ticks}`)
    if ticks >= 5 then
        handle:cancel() -- or task.cancel(handle)
        print("stopped repeating timer")
    end
end)
