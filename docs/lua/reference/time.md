# Reference: time

## Summary

This page is the type reference for the `time` library. The types match the stub `tools/luau_codegen/extra_bindings/task.dluau`.

## Types

```lua
type TimeNamespace = {
    now: () -> number,
    unix: () -> number,
}
```

## Functions

### time.now

```lua
time.now() -> number
```

Returns the seconds since the runtime loaded the task library. It uses a steady clock, which makes it suitable for measuring elapsed time.

### time.unix

```lua
time.unix() -> number
```

Returns the seconds since the unix epoch. It uses the system clock, which makes it suitable for wall clock time.

## Related

- [Tasks and time](../guide/tasks-and-time.md)
- [Reference: task](task.md)

## Source

- `tools/luau_codegen/extra_bindings/task.dluau`
- `src/lua/bindings/task/TaskBinding.cpp`
