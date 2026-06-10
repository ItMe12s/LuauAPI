# Limits and errors

## Summary

Runtime limits and error strings. Values come from `src/core/Config.hpp`.

## Compilation and Bytecode

| Constant | Value | Meaning |
| --- | --- | --- |
| `kMaxCompileDeadlineMs` | `5000 ms` | Max compile time per source before cache insert fails |
| `kMaxBytecodeCacheEntries` | `512` | Max cached compiled scripts |
| `kMaxBytecodeCacheBytes` | `64 MiB` | Max total cached bytecode bytes |

### Compilation and Bytecode errors

| Message | When | Return shape |
| --- | --- | --- |
| `luau compile exceeded 5000 ms budget` | Compile over deadline | Internal. Cache insert fails. |
| (none) | Cache over entry or byte cap | Oldest entries evicted. No Lua error. |

---

## Filesystem

| Constant | Value | Meaning |
| --- | --- | --- |
| `kMaxFsReadBytes` | `32 MiB` | Max `geode.fs.read` payload |
| `kMaxFsWriteBytes` | `32 MiB` | Max `geode.fs.write` payload |
| `kMaxFsListEntries` | `4096` | Max `geode.fs.list` entry count |
| `kMaxFsListNameBytes` | `256 KiB` | Max total name bytes in one listing |

### Filesystem errors

| Message | When | Return shape |
| --- | --- | --- |
| `file exceeds maximum read size` | Read over cap | `nil, err` |
| `data exceeds maximum write size` | Write over cap | `nil, err` |
| `directory listing exceeds maximum entries` | List over entry cap | `nil, err` |
| `directory listing exceeds maximum name bytes` | List over name byte cap | `nil, err` |

---

## Hooks, callbacks, tasks

| Constant | Value | Meaning |
| --- | --- | --- |
| `kMaxHookCallbacksGlobal` | `4096` | Total `geode.hook` callbacks |
| `kMaxHookCallbacksPerTarget` | `64` | Hook callbacks on one target |
| `kMaxScheduledTasks` | `4096` | Tasks alive at once |
| `kMaxCallbackTrampolines` | `4096` soft cap | Orphan callback bridges without an anchor |

### Hooks, callbacks, tasks errors

| Message | When | Return shape |
| --- | --- | --- |
| `geode.hook global callback limit exceeded` | Global hook cap hit | Lua error |
| `geode.hook target callback limit exceeded for %s` | Per-target hook cap hit | Lua error |
| `task: too many scheduled tasks (limit 4096)` | Task cap hit | Lua error |
| (log only) | Trampoline cap hit | Warn once. New trampolines dropped. No Lua error. |

---

## ImGui

| Constant | Value | Meaning |
| --- | --- | --- |
| `kMaxImGuiDrawCallbacks` | `256` | Active `imgui.onDraw` handles at once |

### ImGui errors

| Message | When | Return shape |
| --- | --- | --- |
| `imgui.onDraw: too many draw callbacks (limit 256)` | Draw callback cap hit | Lua error |

---

## JSON limits

| Constant | Value | Meaning |
| --- | --- | --- |
| `kMaxJsonParseBytes` | `8 MiB` | Max input to `geode.json.parse` |
| `kMaxJsonDepth` | `32` | Max nesting depth for Lua and JSON conversion |

### JSON limits errors

| Message | When | Return shape |
| --- | --- | --- |
| `json exceeds maximum size` | Parse input over cap | `nil, err` |
| `json exceeds maximum depth` | Depth over cap | `nil, err` or Lua error on `dump` |

---

## Memory

| Constant | Value | Meaning |
| --- | --- | --- |
| `kMemoryLimitBytes` | `512 MiB` | Hard Lua memory cap |

### Memory errors

| Message | When | Return shape |
| --- | --- | --- |
| (OOM) | Allocation over cap | Lua out of memory error |
| `luau memory limit exceeded` | Internal accounting failure | Log and C++ `Err` on some paths |

---

## Script deadlines for hooks and ImGui

| Constant | Value | Meaning |
| --- | --- | --- |
| `kHookScriptDeadlineMs` | `50 ms` | Budget for hooks, tasks, callbacks, web, and websocket Lua calls |
| `kImGuiScriptDeadlineMs` | `16 ms` | Budget for one `imgui.onDraw` callback |

### Script deadlines for hooks and ImGui errors

| Message | When | Return shape |
| --- | --- | --- |
| `luax: script exceeded %d ms budget` | Run over deadline | Lua error |

---

## Script limits

| Constant | Value | Meaning |
| --- | --- | --- |
| `kDefaultScriptDeadlineMs` | `250 ms` | Default budget for `runScript`, `require`, and module load |
| `kMaxScriptBytes` | `4 MiB` | Max script, module, or `loadstring` source size |

`kDefaultScriptDeadlineMs` is defined in `include/RuntimeTypes.hpp` and exposed through `include/LuauAPI.hpp`.

### Script limits errors

| Message | When | Return shape |
| --- | --- | --- |
| `script exceeds maximum size` | Source over cap | C++ `Err`, `nil, err`, or Lua error |
| `script file exceeds maximum size` | File over cap on C++ run | C++ `Err` |
| `module '%s' exceeds maximum size or cannot be read` | Module file over cap | Lua error on `require` |

---

## Web limits

| Constant | Value | Meaning |
| --- | --- | --- |
| `kMaxWebResponseBytes` | `32 MiB` | Max HTTP response body in Lua |
| `kMaxWebRequestBytes` | `32 MiB` | Max HTTP request or multipart body |

### Web limit serrors

| Message | When | Return shape |
| --- | --- | --- |
| `response exceeds maximum size` | Response body over cap | `nil, err` on accessors and async callbacks |
| `request body exceeds maximum size` | Request body over cap | `nil, err` or Lua error while parsing options |

---

## WebSocket

| Constant | Value | Meaning |
| --- | --- | --- |
| `kMaxWebSocketConnections` | `16` | Max client connections |
| `kMaxWebSocketServers` | `2` | Max listening servers |
| `kMaxWebSocketServerClients` | `32` | Max clients per server |
| `kMaxWebSocketSendBytes` | `8 MiB` | Max outbound message size |
| `kMaxWebSocketReceiveBytes` | `8 MiB` | Max inbound message size |

### WebSocket errors

| Message | When | Return shape |
| --- | --- | --- |
| `too many websocket connections` | Client connection cap hit | `nil, err` on `connect` |
| `too many websocket servers` | Server cap hit | `nil, err` on `serve` |
| `websocket message exceeds maximum send size` | Send over cap | `nil, err` on send methods |
| `websocket message exceeds maximum receive size` | Receive over cap | `onError` callback, close code 1009 |
| (none) | Server client cap hit | Extra clients rejected silently |

---

## How errors reach you

The run functions return `geode::Result<void>`. On failure they return `Err` with a message.

Common cases:

- The function was called off the main thread.
- The runtime is shutting down or not ready.
- The path is empty, absolute, or not a flat `.luau` name.
- The script or file is larger than the size limit.
- The script raised an error or hit its deadline.

You can also read the last runtime error with `lastError`. It is empty off the main thread or while shutting down.

```cpp
namespace lua = imes::luauapi;

if (result.isErr()) {
    log::error("{}", result.unwrapErr());
    auto last = lua::lastError();
    if (!last.empty()) log::error("lastError: {}", last);
}
```

## Deadlines and interrupts

A run that passes its deadline is interrupted and turned into an error.
The check happens at Luau instruction boundaries,
so a very tight loop can run slightly past the deadline before the check fires.

## Memory behavior

The memory cap is hard. There is no soft limit and no extra collection step.
When an allocation would cross the cap, it fails and Lua reports an out of memory error.

## Related

- [API reference](api-reference.md)
- [Integration guide](integration-guide.md)
- [Getting started overview](../../getting-started/overview.md)
- [imgui](../lua/imgui.md)

## Source

- `src/core/Config.hpp`
- `include/RuntimeTypes.hpp`
- `include/LuauAPI.hpp`
- `src/api.cpp`
- `src/core/Runtime.cpp`
