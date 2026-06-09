# Limits and errors

## Summary

The runtime limits and how errors reach the host. The values come from `src/lua/Config.hpp`.

## Limits

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
| Bytecode cache size | `64 MiB` | Total bytes across cached compiled scripts |
| Memory cap | `512 MiB` | Hard Lua memory limit |
| Hook callbacks, global | `4096` | Total hook callbacks across all targets |
| Hook callbacks, per target | `64` | Hook callbacks on one target |
| Scheduled tasks | `4096` | Tasks alive at once |
| Default script deadline | `250 ms` | Budget for a normal run |
| Hook and task deadline | `50 ms` | Budget for a hook or task callback |
| ImGui draw callbacks | `256` | Active `imgui.onDraw` handles at once |
| ImGui draw deadline | `16 ms` | Budget for one ImGui draw callback |
| Menu handler trampolines | `4096` soft cap | Orphan `SEL_MenuHandler` bridges, warns once, never drops |

The default deadline `kDefaultScriptDeadlineMs` is defined in `include/RuntimeTypes.hpp` and exposed
through `include/LuauAPI.hpp`. For ImGui usage, see [imgui](../lua/imgui.md).

## How errors reach you

The run functions return `geode::Result<void>`. On failure they return `Err` with a message. Common
cases:

- The function was called off the main thread.
- The runtime is shutting down or not ready.
- The path is empty, absolute, or not a flat `.luau` name.
- The script or file is larger than the size limit.
- The script raised an error or hit its deadline.

You can also read the last runtime error with `lastError`. It is empty off the main thread or while
shutting down.

```cpp
namespace lua = imes::luauapi;

if (result.isErr()) {
    log::error("{}", result.unwrapErr());
    auto last = lua::lastError();
    if (!last.empty()) log::error("lastError: {}", last);
}
```

## Deadlines and interrupts

A run that passes its deadline is interrupted and turned into an error. The check happens at Luau
instruction boundaries, so a very tight loop can run slightly past the deadline before the check
fires.

## Memory behavior

The memory cap is hard. There is no soft limit and no extra collection step. When an allocation would
cross the cap, it fails and Lua reports an out of memory error.

## Related

- [API reference](api-reference.md)
- [Integration guide](integration-guide.md)
- [Getting started overview](../../getting-started/overview.md)

## Source

- `src/lua/Config.hpp`
- `include/RuntimeTypes.hpp`
- `include/LuauAPI.hpp`
- `src/api.cpp`
- `src/lua/runtime/Runtime.cpp`
