# web

## Summary

`geode.utils.web` exposes Geode async web requests to Lua.
Requests run on the web worker. Callbacks run on the main thread.
See [Getting started](../../getting-started/overview.md) for the shared runtime threading rule.
See Threading below for intercept and listener rules.
This binding does not expose `openLinkUnsafe` or any `*Sync` request method.
File writes stay inside mod sandbox roots.
For signatures, use editor autocomplete from [type stubs](type-stubs.md).
See [Examples](../../getting-started/examples.md).

## Entry points

`geode.utils.web.openLinkInBrowser(url)` opens a URL in the system browser.

`newRequest()` returns a chainable `WebRequest` builder.
Shortcut helpers `get`, `post`, `put`, `patch`, and `fetch` accept a URL, optional `RequestOptions`, and a callback.
The shortcut request functions also accept `callback` as the second argument when no options are needed.
See [globals](globals.md) Error shapes.

`fetch` has three forms:

1. `fetch(url, callback)` - GET to `url`.
2. `fetch(url, options, callback)` - apply `options`, then send. `method` defaults to `GET`.
3. `fetch(options, callback)` - read `url` from `options` (required). `method` defaults to `GET`.

`multipart()` returns a `MultipartForm` builder for file uploads.

Userdata types include `WebRequest`, `WebResponse`, `WebHandle`, `MultipartForm`, and `WebListenerHandle`.
Request timings are in milliseconds.
See Security for `certVerification` and TLS behavior.

## WebRequest

`WebRequest` is a chainable builder.
Set headers, params, method, URL, body, proxy, timeout, progress callback, and other options through fluent methods.
Send methods are async and return a `WebHandle`.
Drop the handle or call `:cancel()` to cancel a pending request.

```lua
local req = geode.utils.web.newRequest()
req:header("Accept", "application/json")
    :timeout(30)
    :get("https://example.com/data", function(response)
        print(response:code())
    end)
```

## WebResponse

Status checks:

- `:info()` - 1xx response.
- `:ok()` - 2xx response.
- `:redirected()` - followed a redirect.
- `:badClient()` - 4xx response.
- `:badServer()` - 5xx response.
- `:error()` - network or transfer error.
- `:cancelled()` - request was cancelled.

Read the body with `:text()`, `:json()`, or `:bytes()`.
`:saveTo(root, path)` writes to a sandbox root (`"save"`, `"config"`, or `"persistent"`).
`:timings()` returns per-phase timing in milliseconds.

Response bodies are capped in the Lua binding after download completes.
Geode does not expose a pre-download or streaming size cap to LuauAPI, so an oversized body may be fully transferred before Luau rejects it.
`:text()`, `:bytes()`, `:json()`, and `:saveTo()` return `nil` and an error when the body is too large.
Async callbacks and response listeners receive `(response?, err?)`.

## WebHandle

`:cancel()` returns true if the request was still pending.
`:id()` returns the Geode request id, or `nil` if there is no active task.

## MultipartForm

Build multipart bodies with `:param`, `:file`, and `:fileFrom`.
`fileFrom` uses sandbox roots only. Raw paths are not allowed.

```lua
local form = geode.utils.web.multipart()
form:param("name", "value")
form:file("upload", bytes, "upload.bin", "application/octet-stream")
form:fileFrom("config", "config", "data.json", "application/json")
```

## Listeners

Register intercept and response listeners with `onRequestIntercept`, `onResponse`, and the `For` / `ById` variants.
Each registrar takes an optional `priority` (higher runs first).
Return `true` to stop other listeners on the main thread.
See Threading below for off-thread behavior.

`onResponse*` callbacks receive `(response?, err?)`, or `(modID, response?, err?)` for the global listener.
Disconnect with `WebListenerHandle:disconnect()`.

```lua
local handle = geode.utils.web.onRequestIntercept(function(modID, request)
    return false
end)

handle:disconnect()
```

## Threading

HTTP work runs on Geode's web worker. Lua callbacks run on the main thread.
See [Getting started](../../getting-started/overview.md) for the shared runtime threading rule.

Web callbacks share the hook callback budget. See [Limits and errors](../cpp/limits-and-errors.md).
Use [tasks and time](tasks.md) to schedule work around callbacks.

Request intercept:

- `onRequestIntercept*` runs on the main thread so Lua can change the request before send.
- Off the main thread, a registered intercept cannot run and the request is blocked. Geode logs a one-time warning.
- If an intercept callback errors, the request is blocked.

Response listeners:

- `onResponse*` may fire on the web worker. Lua runs later on the main thread.
- Off the main thread, listeners are side effects only. They cannot stop other listeners.
- On the main thread, return `true` to stop the next listeners.
- If a response listener callback errors, LuauAPI logs it and keeps response handling best-effort.

Progress:

- `:onProgress` runs on the main thread. Worker events are queued first.

## Limits

Request and response bodies are capped.
These body sources count toward the request cap: `RequestOptions` body fields, `WebRequest` body methods, and `MultipartForm` builders.
Over the cap, failure follows [globals](globals.md) Error shapes.

See [Limits and errors](../cpp/limits-and-errors.md) for body, request, and concurrency caps.

## Security

HTTP requests are not sandboxed. LuauAPI is not an egress firewall.
Any script can call any URL the Geode web stack accepts, including public hosts, private IPs, and localhost.
LuauAPI does not block schemes beyond what Geode allows. There is no URL or host allowlist or blocklist in LuauAPI.

File I/O stays inside mod sandbox roots:

- `:saveTo` on `WebResponse`
- `:fileFrom` on `MultipartForm`

TLS:

- Setting `certVerification` to false disables certificate verification for that request.
- Scripts may disable TLS verification for development, for example self-signed certificates.
- Disabling verification is unsafe for production traffic and should not be used outside trusted dev setups.

Listeners:

- `onRequestIntercept` and `onResponse` see traffic from every loaded mod.
- Callbacks get the mod id as the first argument.
- `onRequestInterceptFor`, `onResponseFor`, and the `ById` variants filter to one mod or request.
- Any mod can still register a global listener.
- Do not put secrets in URLs, headers, or bodies when other mods may be loaded.

For WebSocket LAN exposure with `host = "0.0.0.0"`, see [websocket](websocket.md).
See [LuauAPI mod guidelines](../../mod_guidelines.md) for loadstring and network abuse rules.

## Finding signatures

The authoritative argument lists live in the generated type stubs, surfaced as editor autocomplete.
See [type stubs](type-stubs.md).
Handwritten extras are in `tools/luau_codegen/extra_bindings/web.dluau`.

## Related

- [geode.utils](utils.md)
- [type stubs](type-stubs.md)
- [websocket](websocket.md)
- [fs](fs.md)
- [Getting started](../../getting-started/overview.md)
- [Examples](../../getting-started/examples.md)
- [globals](globals.md)
- [tasks and time](tasks.md)
- [LuauAPI mod guidelines](../../mod_guidelines.md)
- [Limits and errors](../cpp/limits-and-errors.md)

## Source

- `src/bindings/geode/web/GeodeWebCore.cpp`
- `src/bindings/geode/web/GeodeWebApi.cpp`
- `src/bindings/geode/web/GeodeWebListeners.cpp`
- `tools/luau_codegen/extra_bindings/web.dluau`
