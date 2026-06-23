# Limits and errors

## Summary

Runtime limits and error strings. Values come from `src/core/Config.hpp` unless noted.
This page is the canonical list of caps, deadlines, and error strings for LuauAPI.

## Compilation and bytecode

| Constant | Value | Meaning |
| --- | --- | --- |
| `kMaxCompileDeadlineMs` | `15000 ms` | Max compile time per source before cache insert fails |
| `kMaxBytecodeCacheEntries` | `512` | Max cached compiled scripts |
| `kMaxBytecodeCacheBytes` | `64 MiB` | Max total cached bytecode bytes |

### Compilation and bytecode errors

| Message | When | Return shape |
| --- | --- | --- |
| `luau compile exceeded 15000 ms budget` | Compile over deadline | Internal. Cache insert fails. |
| (none) | Cache over entry or byte cap | Oldest entries evicted. No Lua error. |

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

## Hooks, callbacks, tasks

| Constant | Value | Meaning |
| --- | --- | --- |
| `kMaxHookCallbacksGlobal` | `4096` | Total `geode.hook` callbacks |
| `kMaxHookCallbacksPerTarget` | `64` | Hook callbacks on one target |
| `kMaxScheduledTasks` | `4096` | Active (non-cancelled) scheduled tasks at once |
| `kMaxCallbackTrampolines` | `4096` soft cap | Orphan callback bridges without an anchor |

### Hooks, callbacks, tasks errors

| Message | When | Return shape |
| --- | --- | --- |
| `geode.hook global callback limit exceeded` | Global hook cap hit | Lua error |
| `geode.hook target callback limit exceeded for %s` | Per-target hook cap hit | Lua error |
| `task: too many scheduled tasks (limit 4096)` | Task cap hit | Lua error |
| (log only) | Trampoline cap hit | Warn once. New trampolines dropped. No Lua error. |

## ImGui

| Constant | Value | Meaning |
| --- | --- | --- |
| `kMaxImGuiDrawCallbacks` | `256` | Active (non-cancelled) `imgui.onDraw` handles at once |

### ImGui errors

| Message | When | Return shape |
| --- | --- | --- |
| `imgui.onDraw: too many draw callbacks (limit 256)` | Draw callback cap hit | Lua error |
| `imgui.font.add must not run inside an imgui.onDraw callback` | `add` inside draw | Lua error |
| `imgui.font.add must run on the main thread` | `add` off main thread | Lua error |
| `imgui.font.add: size must be greater than 0` | Non-positive size | Lua error |
| `font path must use a .ttf extension` | Non-`.ttf` path | `nil, err` |
| `font file could not be loaded` | Invalid TTF data | `nil, err` |
| `imgui.font.with: font handle is invalid` | Bad or stale handle | Lua error |

## GPU session disable

LuauAPI disables all GPU features for the rest of the game session when the OpenGL context is torn down.
This affects ImGui and `gd3d`.

| Trigger | Effect |
| --- | --- |
| Fullscreen or windowed toggle on Windows | ImGui backend destroyed, live viewports abandoned |
| `TexturesUnloaded` game event | Same as above |

While disabled, ImGui draw callbacks do not run and `gd3d.ViewportFrame` skips draws.
A restart popup appears on the next menu layer. Restart the game to restore GPU features.
There is no Lua error for this path.

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

## Memory

| Constant | Value | Meaning |
| --- | --- | --- |
| `kMemoryLimitBytes` | `512 MiB` | Hard Lua memory cap |

### Memory errors

| Message | When | Return shape |
| --- | --- | --- |
| (OOM) | Allocation over cap | Lua out of memory error |
| `luau memory limit exceeded` | Internal accounting failure | Log and C++ `Err` on some paths |

## Script deadlines for hooks and ImGui

| Constant | Value | Meaning |
| --- | --- | --- |
| `kHookScriptDeadlineMs` | `30000 ms` | Budget for hooks, tasks, callbacks, web, and websocket Lua calls |
| `kImGuiScriptDeadlineMs` | `500 ms` | Budget for one `imgui.onDraw` callback |

### Script deadlines for hooks and ImGui errors

| Message | When | Return shape |
| --- | --- | --- |
| `luax: script exceeded %d ms budget` | Run over deadline | Lua error |

## Script limits

| Constant | Value | Meaning |
| --- | --- | --- |
| `kDefaultScriptDeadlineMs` | `15000 ms` | Default budget for `runScript`, `require`, and module load |
| `kMaxScriptBytes` | `4 MiB` | Max script, module, or `loadstring` source size |

`kDefaultScriptDeadlineMs` is defined in `include/RuntimeTypes.hpp` and exposed through `include/LuauAPI.hpp`.

### Script limits errors

| Message | When | Return shape |
| --- | --- | --- |
| `script exceeds maximum size` | Source over cap | C++ `Err`, `nil, err`, or Lua error |
| `script file exceeds maximum size` | File over cap on C++ run | C++ `Err` |
| `module '%s' exceeds maximum size or cannot be read` | Module file over cap | Lua error on `require` |

## C++ run API

Errors from `runFile`, `runScript`, and their async variants.

| Message | When | Return shape |
| --- | --- | --- |
| `luau api must be called on the main thread` | Sync run off main thread | C++ `Err` |
| `luau runtime shutting down` | Run while shutting down | C++ `Err` |
| `luau runtime not ready` | Runtime failed init | C++ `Err` |
| `luau main-thread execution cancelled` | Async dispatch cancelled | C++ `Err` on future |
| `resources root is empty` | Empty resources root | C++ `Err` |
| `resources root cannot be resolved: ...` | Root path invalid | C++ `Err` |
| `resources root is not a directory: ...` | Root is not a directory | C++ `Err` |
| `relative path must be a flat .luau resource name` | Bad `runFile` relative path | C++ `Err` |
| `script path escapes resources root` | Resolved path outside root | C++ `Err` |
| `script file not found: ...` | Missing script file | C++ `Err` |
| `script cannot be read: ...` | Read failure | C++ `Err` |
| `script exceeds maximum size` | Inline or read source over cap | C++ `Err` |
| (Luau traceback) | Script runtime error or deadline | C++ `Err` and `lastError()` |

Preparation errors do not update `lastError()`.
Execution errors update both `Result` and `lastError()`.

## Web limits

| Constant | Value | Meaning |
| --- | --- | --- |
| `kMaxWebResponseBytes` | `32 MiB` | Max HTTP response body in Lua (enforced after download) |
| `kMaxWebRequestBytes` | `32 MiB` | Max HTTP request or multipart body |
| `kMaxWebConcurrentRequests` | `16` | Max in-flight HTTP requests |

### Web limits errors

| Message | When | Return shape |
| --- | --- | --- |
| `response exceeds maximum size` | Response body over cap | `nil, err` on accessors and async callbacks |
| `request body exceeds maximum size` | Request body over cap | `nil, err` or Lua error while parsing options |
| `too many concurrent web requests` | In-flight request cap hit | Lua error on `get`, `post`, `fetch`, and `WebRequest:send` |

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
| `websocket connection is closed` | Method on closed connection | Lua error or `nil, err` |
| `websocket peer is disconnected` | Method on disconnected peer | `nil, err` |
| `websocket server is stopped` | Method on stopped server | Lua error or `nil, err` |
| `websocket url must start with ws:// or wss://` | Bad client URL scheme | `nil, err` on `connect` |
| `websocket.serve expected port 1..65535` | Port out of range | `nil, err` on `serve` |
| (none) | Server client cap hit | Extra clients rejected silently |

Lifecycle error strings are defined in `src/bindings/websocket/WebSocketInternal.hpp`.

## gd3d and glTF

| Constant | Value | Meaning |
| --- | --- | --- |
| `kMaxFsReadBytes` | `32 MiB` | Max glTF file and buffer read size. Same as the filesystem read cap above. |
| `kMaxProceduralMeshVertices` | `200000` | Max vertices in `gd3d.mesh.new` |
| Max texture dimension | `8192` | Max decoded PNG or JPEG side (`STBI_MAX_DIMENSIONS` in `ImageDecode.cpp`) |

### gd3d and glTF errors

| Message | When | Return shape |
| --- | --- | --- |
| `path is not a regular file` | `loadMesh` target is not a file | `nil, err` |
| `glTF file exceeds maximum read size` | File over read cap | `nil, err` |
| `glTF file cannot be read: ...` | Open or read failure | `nil, err` |
| `glTF file cannot be opened: ...` | Open failure | `nil, err` |
| `glTF data is empty` | Empty buffer | `nil, err` |
| `glTF data exceeds maximum read size` | In-memory data over cap | `nil, err` |
| `failed to parse glTF: ...` | cgltf parse failure | `nil, err` |
| `failed to load glTF buffers: ...` | Buffer load failure | `nil, err` |
| `glTF file contains no mesh primitives` | No geometry | `nil, err` |
| `only triangle primitives are supported` | Non-triangle primitive | `nil, err` |
| `Draco compressed primitives are not supported` | Draco extension | `nil, err` |
| `sparse accessors are not supported` | Sparse accessor | `nil, err` |
| `meshopt-compressed accessors are not supported` | meshopt compression | `nil, err` |
| `textures require TEXCOORD_0` | Textured material without UVs | `nil, err` |
| `KHR texture extensions are not supported` | BasisU, WebP, and similar | `nil, err` |
| `failed to create ViewportFrame` | Viewport node creation failed | `nil, err` |
| `%s: mesh handle is invalid` | Stale or bad mesh userdata | Lua error |
| `ViewportFrame:addMesh: mesh handle is invalid` | Mesh released before add | Lua error |
| `%s: material handle is invalid` | Stale or bad material userdata | Lua error |
| `gd3d.Material.new: expected color field` | Missing color in constructor table | Lua error |
| `positions exceed maximum vertex count` | Procedural mesh over vertex cap | `nil, err` on `gd3d.mesh.new` |
| `encoded image exceeds maximum read size` | Image file over read cap | `nil, err` on texture or glTF image load |
| `failed to decode image: ...` | stb decode failure | `nil, err` |
| `decoded image exceeds maximum size` | Decoded pixel buffer over cap | `nil, err` |

Sandbox path errors from [fs](../lua/fs.md) also apply to `loadMesh` roots and paths.

## Crash sidecar

| Constant | Value | Meaning |
| --- | --- | --- |
| `kSidecarStackDepth` | `64` | Max boundary frames kept in memory |
| `kSidecarFlushIntervalMs` | `100 ms` | Min time between interval-driven disk flushes |

Sidecar files: `luauapi-last-context.txt` and temp `luauapi-last-context.tmp` in the Geode crashlogs dir.
See [Crash sidecar](../../contributor/internals/crash-sidecar.md).

## How errors reach you

The run functions return `geode::Result<void>`. On failure they return `Err` with a message.

Common cases include off-main-thread calls, shutdown, bad paths, oversized scripts, script errors, and deadline overruns.

Read the message from `Result` and optionally `lastError()`.
See [Your first script](../../getting-started/first-script.md) and [C++ API reference](api-reference.md).

## Deadlines and interrupts

A run that passes its deadline is interrupted and turned into an error.
The check happens at Luau instruction boundaries,
so a very tight loop can run slightly past the deadline before the check fires.

Defaults are generous so scripts can finish under load. For C++ callers, pass a lower `deadlineMs` when you can.
The runtime is shared with hooks, tasks, and ImGui, so keep custom deadlines under 100 ms when possible.
For `imgui.onDraw`, stay under 20 ms so frame time and menu input stay usable.

## Memory behavior

The memory cap is hard. There is no soft limit and no extra collection step.
When an allocation would cross the cap, it fails and Lua reports an out of memory error.

## Related

- [Getting started](../../getting-started/overview.md)
- [LuauAPI mod guidelines](../../mod_guidelines.md)
- [C++ API reference](api-reference.md)
- [imgui](../lua/imgui.md)
- [gd3d](../lua/gd3d.md)
- [hooks](../lua/hooks.md)
- [tasks and time](../lua/tasks.md)
- [web](../lua/web.md)
- [websocket](../lua/websocket.md)

## Source

- `src/core/Config.hpp`
- `src/bindings/imgui/ImGuiStyleFonts.cpp`
- `src/bindings/imgui/ImGuiFontRegistry.hpp`
- `src/render3d/gpu/GpuSessionDisable.cpp`
- `src/bindings/geode/web/WebCaps.hpp`
- `src/bindings/websocket/WebSocketInternal.hpp`
- `src/render3d/assets/MeshAsset.hpp`
- `src/render3d/assets/ImageDecode.cpp`
- `include/RuntimeTypes.hpp`
- `include/LuauAPI.hpp`
- `src/api.cpp`
- `src/core/Runtime.cpp`
