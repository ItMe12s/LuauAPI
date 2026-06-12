# web

## Summary

`geode.utils.web` exposes Geode's async web request system to Lua.
Requests run on Geode's web worker and call Lua completion and progress callbacks on the main thread.
This binding does not expose `openLinkUnsafe` or any `*Sync` request method.
File writes are sandboxed to mod roots.
See [Examples](../../getting-started/examples.md).

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

Table types:

- `ProxyOpts`
- `WebProgress`
- `RequestTimings`

Userdata types:

- `WebRequest`
- `WebResponse`
- `WebHandle`
- `MultipartForm`
- `WebListenerHandle`
  
Timings are milliseconds.
Setting `certVerification` to false disables TLS verification for that request, an intentional escape hatch.

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

`WebRequest` is a chainable builder that supports:

- headers
- query parameters
- proxy options
- timeouts
- range requests
- body data
- JSON bodies
- multipart bodies
- progress callbacks
- request introspection

Send methods are async and return a `WebHandle`.
Dropping the handle or calling `:cancel()` cancels a still-pending request.

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

The writable roots accepted by `saveTo` are:

- `"save"`
- `"config"`
- `"persistent"`

Response bodies are capped.

These methods return `nil` and an error message when the body exceeds the cap:

- `:text()`
- `:bytes()`
- `:json()`
- `:saveTo()`

Async callbacks and response listeners receive `(response?, err?)`.
An oversized body arrives as `nil` and an error message.

## MultipartForm

```lua
local form = geode.utils.web.multipart()
form:param("name", "value")
form:file("upload", bytes, "upload.bin", "application/octet-stream")
form:fileFrom("config", "config", "data.json", "application/json")
```

`fileFrom` uses sandbox roots. Raw filesystem paths are not exposed.

## Request body caps

The following body sources are capped.

- `RequestOptions.body`
- `RequestOptions.bodyString`
- `RequestOptions.bodyJson`
- `RequestOptions.bodyMultipart`
- `WebRequest:body`
- `WebRequest:bodyString`
- `WebRequest:bodyJson`
- `WebRequest:bodyMultipart`
- `MultipartForm:param`
- `MultipartForm:file`
- `MultipartForm:fileFrom`
- `MultipartForm:getBody()`

Over the [request and response body caps](../cpp/limits-and-errors.md),
the call returns `nil` and an error message, or raises while parsing options.

## Listeners

```lua
local handle = geode.utils.web.onRequestIntercept(function(modID, request)
    return false
end)

handle:disconnect()
```

Request intercept callbacks may return `true` to stop propagation when Geode fires the event on the main thread.
Intercept handlers run right away so Lua can tweak the request as it happens.
If the event fires off the main thread, Lua is skipped, the event continues, and you see a one-time warning.

A response listener that would run on the web worker is delayed until the main thread.
Its return value cannot stop other listeners, so use these mainly for side effects.
When Geode fires the response event on the main thread, returning `true` still stops the next listeners.

Available listeners:

- `onRequestIntercept`
- `onRequestInterceptFor`
- `onRequestInterceptById`
- `onResponse`
- `onResponseFor`
- `onResponseById`

Each accepts an optional `priority` argument.
Response callbacks receive `(response?, err?)` or `(modID, response?, err?)`

## Constants

`HttpVersion`, `ProxyType`, `Error`, and `HttpAuth` mirror Geode `utils/web.hpp` constants.

## Limits

Request and response bodies are capped.

See [Limits and errors](../cpp/limits-and-errors.md).

## Related

- [json](json.md)
- [fs](fs.md)
- [Globals](globals.md)

## Source

- `src/bindings/geode/web/GeodeWebBinding.cpp`
- `src/bindings/geode/web/GeodeWebApply.cpp`
- `src/bindings/geode/web/GeodeWebOptions.cpp`
- `src/bindings/geode/web/GeodeWebRequest.cpp`
- `src/bindings/geode/web/GeodeWebResponse.cpp`
- `src/bindings/geode/web/GeodeWebMultipart.cpp`
- `src/bindings/geode/web/GeodeWebListeners.cpp`
- `tools/luau_codegen/extra_bindings/web.dluau`
