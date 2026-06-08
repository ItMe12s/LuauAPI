# Limits and errors

## Summary

This page lists the runtime limits and explains how errors reach the host.

## Limits

These values come from `src/lua/Config.hpp`.

| Limit | Value | Meaning |
| --- | --- | --- |
| Max script size | `4 MiB` | Largest single script or module |
| Max filesystem read/write | `32 MiB` | Largest `geode.fs.read` or `geode.fs.write` payload |
| Max web response body | `32 MiB` | Largest body boxed as `WebResponse` |
| Max web request body | `32 MiB` | Largest request or multipart body payload |
| Max JSON parse size | `8 MiB` | Largest string accepted by JSON parsing helpers |
| Max JSON depth | `32` | Deepest JSON value converted between Lua and C++ |
| Max directory entries | `4096` | Largest `geode.fs.list` entry count |
| Max directory name bytes | `256 KiB` | Total returned names from `geode.fs.list` |
| Bytecode cache entries | `512` | Cached compiled scripts, least recently used dropped |
| Memory cap | `512 MiB` | Hard Lua memory limit |
| Hook callbacks, global | `4096` | Total hook callbacks across all targets |
| Hook callbacks, per target | `64` | Hook callbacks on one target |
| Scheduled tasks | `4096` | Tasks alive at once |
| Default script deadline | `250 ms` | Budget for a normal run |
| Hook and task deadline | `50 ms` | Budget for a hook or task callback |
| ImGui draw callbacks | `256` | Active `imgui.onDraw` handles at once |
| Menu handler trampolines | `4096` soft cap | Orphan `SEL_MenuHandler` bridges. Warns once when exceeded, never drops |
| ImGui draw deadline | `16 ms` | Budget for one ImGui draw callback |

The public default deadline is also defined in `include/LuauAPI.hpp` as `kDefaultScriptDeadlineMs`.

For ImGui limits and usage, see [Reference: imgui](../lua/reference/imgui.md).

## How errors reach you

The run functions return `geode::Result<void>`. On failure they return `Err` with a message.
The examples below use the alias

```cpp
namespace lua = imes::luauapi;
```

Common messages include the following cases:

- The function was called off the main thread.
- The runtime is shutting down.
- The runtime is not ready.
- The path is empty, absolute, or not a flat `.luau` name.
- The script or file is larger than the size limit.
- The script raised an error or hit its deadline.

You can also read the last runtime error with `lastError`. It is empty off the main thread or while shutting down.

```cpp
if (result.isErr()) {
    log::error("{}", result.unwrapErr());
    auto last = lua::lastError();
    if (!last.empty()) log::error("lastError: {}", last);
}
```

## Deadlines and interrupts

A run that passes its deadline is interrupted and turned into an error.
The check happens at Luau instruction boundaries, so a very tight loop can run slightly past the deadline before the check fires.

## Memory behavior

The memory cap is hard. There is no soft limit and no extra collection step.
When an allocation would cross the cap, it fails and Lua reports an out of memory error.

## Related

- [API reference](api-reference.md)
- [Core concepts](../getting-started/chapter-5.md)

## Source

- `src/lua/Config.hpp`
- `include/LuauAPI.hpp`
- `src/api.cpp`
- `src/lua/runtime/Runtime.cpp`
