# Reference: web

## Summary

`geode.utils.web` exposes Geode's async web request system to Lua.
Requests run on Geode's web worker and call Lua completion/progress callbacks on the main thread.

This binding does not expose `openLinkUnsafe` or any `*Sync` request method.
File writes are sandboxed to mod roots.

## Types

```lua
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
    version: number?,
    body: string?,
    bodyString: string?,
    bodyJson: any?,
    bodyMultipart: MultipartForm?,
    onProgress: ((progress: WebProgress) -> ())?,
}
```

`ProxyOpts`, `WebProgress`, and `RequestTimings` are table types in the generated stubs.
`WebRequest`, `WebResponse`, `WebHandle`, `MultipartForm`, and `WebListenerHandle` are userdata.
Timings are milliseconds.
Setting `certVerification` to false disables TLS certificate verification for that request.
This is intentional escape-hatch behavior for scripts that own the risk.

## Functions

```lua
geode.utils.web.openLinkInBrowser(url: string) -> ()
geode.utils.web.newRequest() -> WebRequest
geode.utils.web.multipart() -> MultipartForm
geode.utils.web.get(url: string, options: RequestOptions?, callback: (response: WebResponse?, err: string?) -> ()) -> WebHandle
geode.utils.web.post(url: string, options: RequestOptions?, callback: (response: WebResponse?, err: string?) -> ()) -> WebHandle
geode.utils.web.put(url: string, options: RequestOptions?, callback: (response: WebResponse?, err: string?) -> ()) -> WebHandle
geode.utils.web.patch(url: string, options: RequestOptions?, callback: (response: WebResponse?, err: string?) -> ()) -> WebHandle
geode.utils.web.fetch(urlOrOptions, optionsOrCallback, callback?) -> WebHandle
```

The shortcut request functions also accept `callback` as the second argument when no options are needed.

## WebRequest

`WebRequest` is a chainable builder. It supports:

- headers
- query params
- proxy options
- timeouts
- range requests
- body data
- JSON bodies
- multipart bodies
- progress callbacks
- request introspection

```lua
local req = geode.utils.web.newRequest()
req:header("Accept", "application/json")
    :timeout(30)
    :get("https://example.com/data", function(response)
        print(response:code())
    end)
```

Send methods are async and return a `WebHandle`.
Dropping the handle or calling `:cancel()` cancels the request if it is still pending.

## WebResponse

```lua
response:ok() -> boolean
response:code() -> number
response:text() -> (string?, string?)
response:json() -> (any?, string?)
response:bytes() -> string
response:saveTo(root: FsRoot, path: string) -> (boolean?, string?)
response:headers() -> { string }
response:header(name: string) -> string?
response:headersNamed(name: string) -> { string }?
response:errorMessage() -> string
response:verboseLogs() -> string
response:timings() -> RequestTimings
```

`saveTo` accepts only writable roots: `"save"`, `"config"`, and `"persistent"`.

Response bodies are capped at 32 MB. `:text()`, `:bytes()`, `:json()`, and `:saveTo()`
return `nil` and an error message when the body exceeds the cap.

Async request callbacks and response listeners receive `(response?, err?)`.
When a response body exceeds the cap, Lua gets `nil` and `"response exceeds maximum size"`
instead of a boxed `WebResponse`.

The following sources of request bodies are capped at 32 MB:

- `RequestOptions.body`
- `RequestOptions.bodyString`
- `RequestOptions.bodyMultipart`
- `WebRequest:body`
- `WebRequest:bodyString`
- `WebRequest:bodyMultipart`
- `MultipartForm:file`
- `MultipartForm:fileFrom`
- `MultipartForm:getBody()`

If request data exceeds the limit, the function either returns `nil` and `"request body exceeds maximum size"`
or parses the options and raises that error.

## MultipartForm

```lua
local form = geode.utils.web.multipart()
form:param("name", "value")
form:file("upload", bytes, "upload.bin", "application/octet-stream")
form:fileFrom("config", "config", "data.json", "application/json")
```

`fileFrom` uses sandbox roots. Raw filesystem paths are not exposed.

## Events

```lua
local handle = geode.utils.web.onRequestIntercept(function(modID, request)
    return false
end)

handle:disconnect()
```

Request intercept callbacks may return `true` to stop propagation when Geode fires the event on the main thread.
If an intercept event fires off the main thread, LuauAPI does not call Lua and lets the event propagate.

Response listeners from Geode's web worker are queued back to the main thread before Lua is called,
so their return value is best treated as informational.

Available listeners:

- `onRequestIntercept(callback, priority?)`
- `onRequestInterceptFor(modID, callback, priority?)`
- `onRequestInterceptById(requestID, callback, priority?)`
- `onResponse(callback, priority?)`
- `onResponseFor(modID, callback, priority?)`
- `onResponseById(requestID, callback, priority?)`

Response listener callbacks receive `(response?, err?)` or `(modID, response?, err?)`.
Oversized bodies arrive as `nil` plus `"response exceeds maximum size"`.

## Constants

`HttpVersion`, `ProxyType`, `Error`, and `HttpAuth` mirror Geode `utils/web.hpp` constants.

## Example

```lua
local web = geode.utils.web

local handle = web.get("https://api.example.com/data", {
    headers = { Accept = "application/json" },
    timeout = 30,
    onProgress = function(progress)
        print(progress.downloadProgress)
    end,
}, function(response)
    if not response:ok() then
        print(response:errorMessage())
        return
    end

    local data, err = response:json()
    if err then
        print(err)
        return
    end

    print(data.version)
end)

-- handle:cancel()
```

## Related

- [Reference: json](json.md)
- [Reference: fs](fs.md)
- [Lua overview](../overview.md)

## Source

- `src/lua/bindings/geode/GeodeWebBinding.cpp`
- `src/lua/bindings/geode/GeodeWebOptions.cpp`
- `src/lua/bindings/geode/GeodeWebRequest.cpp`
- `src/lua/bindings/geode/GeodeWebResponse.cpp`
- `src/lua/bindings/geode/GeodeWebMultipart.cpp`
- `src/lua/bindings/geode/GeodeWebListeners.cpp`
- `src/lua/bindings/geode/WebCaps.hpp`
- `src/lua/bindings/geode/WebInternal.hpp`
- `tools/luau_codegen/extra_bindings/web.dluau`
- `tools/luau_codegen/emit/luau_types/manual_fields.py`
