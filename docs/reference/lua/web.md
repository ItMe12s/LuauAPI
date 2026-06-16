# web

## Summary

`geode.utils.web` exposes Geode async web requests to Lua.
Requests run on the web worker. Callbacks run on the main thread.
See Threading below for intercept and listener rules.
This binding does not expose `openLinkUnsafe` or any `*Sync` request method.
File writes stay inside mod sandbox roots.
See [Examples](../../getting-started/examples.md).

## Types

```lua
type ProxyOpts = {
    address: string,
    port: number?,
    type: number?, -- ProxyType
    auth: number?, -- HttpAuth
    username: string?,
    password: string?,
    tunneling: boolean?,
    certVerification: boolean?,
}

type RequestTimings = {
    queueWait: number,
    nameLookup: number,
    connect: number,
    tlsHandshake: number,
    requestSend: number,
    firstByte: number,
    download: number,
    total: number,
}

type WebProgress = {
    downloaded: number,
    downloadTotal: number,
    downloadProgress: number?,
    uploaded: number,
    uploadTotal: number,
    uploadProgress: number?,
}

type RequestOptions = {
    url: string?,
    method: string?,
    headers: { [string]: string }?,
    params: { [string]: string }?,
    userAgent: string?,
    acceptEncoding: string?,
    timeout: number?,
    downloadRange: { number }?,
    certVerification: boolean?,
    transferBody: boolean?,
    followRedirects: boolean?,
    ignoreContentLength: boolean?,
    caBundle: string?,
    proxy: ProxyOpts?,
    version: number?, -- HttpVersion
    body: string?,
    bodyString: string?,
    bodyJson: any?,
    bodyMultipart: MultipartForm?,
    onProgress: ((progress: WebProgress) -> ())?,
}
```

Userdata types:

- `WebRequest`
- `WebResponse`
- `WebHandle`
- `MultipartForm`
- `WebListenerHandle`

Timings are in milliseconds.
See Security for `certVerification` and TLS behavior.

## Functions

```lua
geode.utils.web.openLinkInBrowser(url: string) -> ()
geode.utils.web.newRequest() -> WebRequest
geode.utils.web.multipart() -> MultipartForm
geode.utils.web.get(url: string, options: RequestOptions?, callback: (response: WebResponse?, err: string?) -> ()) -> WebHandle
geode.utils.web.post(url: string, options: RequestOptions?, callback: (response: WebResponse?, err: string?) -> ()) -> WebHandle
geode.utils.web.put(url: string, options: RequestOptions?, callback: (response: WebResponse?, err: string?) -> ()) -> WebHandle
geode.utils.web.patch(url: string, options: RequestOptions?, callback: (response: WebResponse?, err: string?) -> ()) -> WebHandle
geode.utils.web.fetch(url: string, callback: (response: WebResponse?, err: string?) -> ()) -> WebHandle
geode.utils.web.fetch(url: string, options: RequestOptions, callback: (response: WebResponse?, err: string?) -> ()) -> WebHandle
geode.utils.web.fetch(options: RequestOptions, callback: (response: WebResponse?, err: string?) -> ()) -> WebHandle
```

The shortcut request functions also accept `callback` as the second argument when no options are needed.
See [Globals](globals.md) Error shapes.

`fetch` has three forms:

1. `fetch(url, callback)` - GET to `url`.
2. `fetch(url, options, callback)` - apply `options`, then send. `method` defaults to `GET`.
3. `fetch(options, callback)` - read `url` from `options` (required). `method` defaults to `GET`.

## WebRequest

```lua
request:header(name: string, value: string) -> WebRequest
request:removeHeader(name: string) -> WebRequest
request:param(name: string, value: string) -> WebRequest
request:removeParam(name: string) -> WebRequest
request:method(method: string) -> WebRequest
request:url(url: string) -> WebRequest
request:userAgent(name: string) -> WebRequest
request:acceptEncoding(encoding: string) -> WebRequest
request:timeout(seconds: number) -> WebRequest
request:downloadRange(start: number, stop: number) -> WebRequest
request:certVerification(enabled: boolean) -> WebRequest
request:transferBody(enabled: boolean) -> WebRequest
request:followRedirects(enabled: boolean) -> WebRequest
request:ignoreContentLength(enabled: boolean) -> WebRequest
request:caBundle(content: string) -> WebRequest
request:proxy(opts: ProxyOpts) -> WebRequest
request:version(version: number) -> WebRequest
request:body(data: string) -> (WebRequest?, string?)
request:bodyString(data: string) -> (WebRequest?, string?)
request:bodyJson(value: any) -> (WebRequest?, string?)
request:bodyMultipart(form: MultipartForm) -> (WebRequest?, string?)
request:onProgress(callback: (progress: WebProgress) -> ()) -> WebRequest
request:send(method: string, url: string, callback: (response: WebResponse?, err: string?) -> ()) -> WebHandle
request:send(method: string, url: string, options: RequestOptions, callback: (response: WebResponse?, err: string?) -> ()) -> WebHandle
request:get(url: string, callback: (response: WebResponse?, err: string?) -> ()) -> WebHandle
request:get(url: string, options: RequestOptions, callback: (response: WebResponse?, err: string?) -> ()) -> WebHandle
request:post(url: string, callback: (response: WebResponse?, err: string?) -> ()) -> WebHandle
request:post(url: string, options: RequestOptions, callback: (response: WebResponse?, err: string?) -> ()) -> WebHandle
request:put(url: string, callback: (response: WebResponse?, err: string?) -> ()) -> WebHandle
request:put(url: string, options: RequestOptions, callback: (response: WebResponse?, err: string?) -> ()) -> WebHandle
request:patch(url: string, callback: (response: WebResponse?, err: string?) -> ()) -> WebHandle
request:patch(url: string, options: RequestOptions, callback: (response: WebResponse?, err: string?) -> ()) -> WebHandle
request:getID() -> number
request:getMethod() -> string
request:getUrl() -> string
request:getHeaders() -> { [string]: { string } }
request:getUrlParams() -> { [string]: string }
request:getBody() -> string?
request:getTimeout() -> number?
request:getVersion() -> number
request:getProgress() -> WebProgress
```

`WebRequest` is a chainable builder.
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

```lua
response:info() -> boolean
response:ok() -> boolean
response:redirected() -> boolean
response:badClient() -> boolean
response:badServer() -> boolean
response:error() -> boolean
response:cancelled() -> boolean
response:code() -> number
response:text() -> (string?, string?)
response:json() -> (any?, string?)
response:bytes() -> (string?, string?)
response:saveTo(root: FsRoot, path: string) -> (boolean?, string?)
response:headers() -> { string }
response:header(name: string) -> string?
response:headersNamed(name: string) -> { string }?
response:errorMessage() -> string
response:verboseLogs() -> string
response:timings() -> RequestTimings
```

Status checks:

- `:info()` - 1xx response.
- `:ok()` - 2xx response.
- `:redirected()` - followed a redirect.
- `:badClient()` - 4xx response.
- `:badServer()` - 5xx response.
- `:error()` - network or transfer error.
- `:cancelled()` - request was cancelled.

`saveTo` accepts `"save"`, `"config"`, and `"persistent"`.
Response bodies are capped in the Lua binding after download completes.
Geode does not expose a pre-download or streaming size cap to LuauAPI, so an oversized body may be fully transferred before Luau rejects it.
`:text()`, `:bytes()`, `:json()`, and `:saveTo()` return `nil` and an error when the body is too large.
Async callbacks and response listeners receive `(response?, err?)`.

## WebHandle

```lua
handle:cancel() -> boolean
handle:id() -> number?
```

`:cancel()` returns true if the request was still pending.
`:id()` returns the Geode request id, or `nil` if there is no active task.

## MultipartForm

```lua
form:param(name: string, value: string) -> (MultipartForm?, string?)
form:file(name: string, data: string, filename: string, mime: string?) -> (MultipartForm?, string?)
form:fileFrom(name: string, root: FsRoot, path: string, mime: string?) -> (MultipartForm?, string?)
form:getBoundary() -> string
form:getHeader() -> string
form:getBody() -> (string?, string?)
```

`fileFrom` uses sandbox roots only. Raw paths are not allowed.

```lua
local form = geode.utils.web.multipart()
form:param("name", "value")
form:file("upload", bytes, "upload.bin", "application/octet-stream")
form:fileFrom("config", "config", "data.json", "application/json")
```

## WebListenerHandle

```lua
handle:disconnect() -> ()
```

Disconnects a listener registered by `onRequestIntercept`, `onResponse`, or the `For` / `ById` variants.

## Listeners

```lua
geode.utils.web.onRequestIntercept(callback: (modID: string, request: WebRequest) -> boolean?, priority: number?) -> WebListenerHandle
geode.utils.web.onRequestInterceptFor(modID: string, callback: (request: WebRequest) -> boolean?, priority: number?) -> WebListenerHandle
geode.utils.web.onRequestInterceptById(requestID: number, callback: (request: WebRequest) -> boolean?, priority: number?) -> WebListenerHandle
geode.utils.web.onResponse(callback: (modID: string, response: WebResponse?, err: string?) -> boolean?, priority: number?) -> WebListenerHandle
geode.utils.web.onResponseFor(modID: string, callback: (response: WebResponse?, err: string?) -> boolean?, priority: number?) -> WebListenerHandle
geode.utils.web.onResponseById(requestID: number, callback: (response: WebResponse?, err: string?) -> boolean?, priority: number?) -> WebListenerHandle
```

Return `true` to stop other listeners on the main thread.
See Threading below for off-thread behavior.

Each registrar takes an optional `priority` (higher runs first).
`onResponse*` callbacks receive `(response?, err?)`, or `(modID, response?, err?)` for the global listener.

```lua
local handle = geode.utils.web.onRequestIntercept(function(modID, request)
    return false
end)

handle:disconnect()
```

## Threading

HTTP work runs on Geode's web worker. Lua callbacks run on the main thread.

Web callbacks share the hook callback budget (30000 ms). See [Limits and errors](../cpp/limits-and-errors.md).
Use [tasks](tasks.md) to schedule work around callbacks.

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

## HttpVersion

```lua
geode.utils.web.HttpVersion -> {
    DEFAULT: number,
    VERSION_1_0: number,
    VERSION_1_1: number,
    VERSION_2_0: number,
    VERSION_2TLS: number,
    VERSION_2_PRIOR_KNOWLEDGE: number,
    VERSION_3: number,
    VERSION_3ONLY: number,
}
```

HTTP version constants from Geode `utils/web.hpp`.

## ProxyType

```lua
geode.utils.web.ProxyType -> {
    HTTP: number,
    HTTPS: number,
    HTTPS2: number,
    SOCKS4: number,
    SOCKS4A: number,
    SOCKS5: number,
    SOCKS5H: number,
}
```

Proxy type constants from Geode `utils/web.hpp`.

## Error

```lua
geode.utils.web.Error -> {
    CURL_INITIALIZATION_ERROR: number,
    REQUEST_CANCELLED: number,
    QUEUE_FULL: number,
    CHANNEL_CLOSED: number,
}
```

Web worker error constants from Geode `utils/web.hpp`.

## HttpAuth

```lua
geode.utils.web.HttpAuth -> {
    BASIC: number,
    DIGEST: number,
    DIGEST_IE: number,
    BEARER: number,
    NEGOTIATE: number,
    NTLM: number,
    NTLM_WB: number,
    ANY: number,
    ANYSAFE: number,
    ONLY: number,
    AWS_SIGV4: number,
}
```

HTTP auth scheme constants from Geode `utils/web.hpp`.

## Limits

Request and response bodies are capped. These body sources count toward the request cap:

- `RequestOptions.body`, `bodyString`, `bodyJson`, `bodyMultipart`
- `WebRequest:body`, `bodyString`, `bodyJson`, `bodyMultipart`
- `MultipartForm:param`, `file`, `fileFrom`, `getBody()`

Over the cap, the call returns `nil` and an error, or raises while parsing options.

At most 16 HTTP requests may be in flight at once.
Additional starts raise `too many concurrent web requests` before the request is sent.

See [Limits and errors](../cpp/limits-and-errors.md).

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

## Related

- [Getting started overview](../../getting-started/overview.md)
- [Examples](../../getting-started/examples.md)
- [LuauAPI mod guidelines](../../mod_guidelines.md)
- [geode.utils](utils.md)
- [websocket](websocket.md)
- [json](json.md)
- [fs](fs.md)
- [Tasks and time](tasks.md)
- [Globals](globals.md)
- [Limits and errors](../cpp/limits-and-errors.md)

## Source

- `src/bindings/geode/web/GeodeWebBinding.cpp`
- `src/bindings/geode/web/GeodeWebApply.cpp`
- `src/bindings/geode/web/GeodeWebOptions.cpp`
- `src/bindings/geode/web/GeodeWebRequest.cpp`
- `src/bindings/geode/web/GeodeWebResponse.cpp`
- `src/bindings/geode/web/GeodeWebMultipart.cpp`
- `src/bindings/geode/web/GeodeWebListeners.cpp`
- `tools/luau_codegen/extra_bindings/web.dluau`
