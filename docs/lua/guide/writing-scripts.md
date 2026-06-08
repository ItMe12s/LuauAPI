# Writing scripts

## Summary

This page covers the basics of a script: logging, error handling, the time budget, and return values.

## Logging with print

`print` writes a single line to the Geode log, and each argument is separated by a tab.

```lua
print("loaded", 1, true)
```

## Errors are logged, not fatal

The host runs your script inside a protected call.
When your script raises an error, the runtime catches it and writes it to the log, and the game keeps running.

The host can read the last error after a run. See [Limits and errors](../cpp/limits-and-errors.md).

## The time budget

Every run has a deadline in milliseconds. When a script runs past its deadline, the runtime stops it and raises an error.

A normal run uses the default deadline. Hook and task callbacks get a shorter budget.
Avoid long loops and blocking calls. Spread heavy work across frames with `task`.
See [Limits and errors](../cpp/limits-and-errors.md) for all deadline values.

## Memory

The runtime caps Lua memory at a hard limit. Once that cap is reached, allocation fails.
Keep large data small and release references you no longer need.
See [Limits and errors](../cpp/limits-and-errors.md).

## Returning a value from a module

A file loaded with `require` must return exactly one value. A top level script run with `runFile` does not need to return anything.

```lua
-- a module file
local M = {}
function M.greet()
    print("hihi :)")
end
return M
```

## Related

- [Tasks and time](tasks-and-time.md)
- [Modules and require](modules-and-require.md)
- [Core concepts](../getting-started/chapter-5.md)

## Source

- `src/lua/runtime/Runtime.cpp`
- `src/lua/Config.hpp`
- `src/lua/module/Requirer.cpp`
