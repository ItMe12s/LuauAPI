# websocket

## Summary

The `websocket` library provides WebSocket client connections and a local WebSocket server, backed by IXWebSocket.
Sockets run on background threads and deliver all events to Lua on the main thread.
The client supports `ws://` and `wss://` (TLS via mbedTLS). The server is plain `ws://` only.

## Types

```lua
type WebSocketConnectOptions = {
    headers: { [string]: string }?,
    protocols: { string }?,
    pingIntervalSecs: number?,
    handshakeTimeoutSecs: number?,
    autoReconnect: boolean?,    -- default false
    certVerification: boolean?, -- default true
    caBundle: string?,          -- PEM bundle content, in-memory
}

type WebSocketServeOptions = {
    host: string?, -- default "127.0.0.1"
}

type WebSocketReadyState = "connecting" | "open" | "closing" | "closed"
```

Userdata types:

- `WebSocketConnection`
- `WebSocketServer`
- `WebSocketPeer`

## Functions

```lua
websocket.connect(url: string, options: WebSocketConnectOptions?) -> (WebSocketConnection?, string?)
websocket.serve(port: number, options: WebSocketServeOptions?) -> (WebSocketServer?, string?)
```

`connect` starts the socket immediately and returns the connection.
Events arrive on the next frame at the earliest, so attaching callbacks right after `connect` never misses events.
`serve` binds and starts listening, or returns `nil` and an error message. See [Globals](globals.md) Error shapes.

## WebSocketConnection

```lua
ws:send(data: string) -> (boolean?, string?)
ws:sendBinary(data: string) -> (boolean?, string?)
ws:ping(payload: string?) -> (boolean?, string?)
ws:close(code: number?, reason: string?) -> ()
ws:readyState() -> WebSocketReadyState
ws:url() -> string
ws:onOpen(callback: () -> ()) -> WebSocketConnection
ws:onMessage(callback: (data: string, isBinary: boolean) -> ()) -> WebSocketConnection
ws:onClose(callback: (code: number, reason: string, remote: boolean) -> ()) -> WebSocketConnection
ws:onError(callback: (message: string) -> ()) -> WebSocketConnection
```

Callback setters return the connection for chaining.
Automatic reconnection is disabled unless `autoReconnect = true` is passed.
Dropping the last reference to a connection closes it on garbage collection.

```lua
local ws = websocket.connect("wss://echo.websocket.org")
ws:onOpen(function()
    ws:send("hello")
end):onMessage(function(data, isBinary)
    print("received:", data)
end):onClose(function(code, reason, remote)
    print("closed:", code, reason)
end):onError(function(message)
    print("error:", message)
end)
```

## WebSocketServer

```lua
server:broadcast(data: string) -> (boolean?, string?)
server:broadcastBinary(data: string) -> (boolean?, string?)
server:clients() -> { WebSocketPeer }
server:port() -> number
server:stop() -> ()
server:onClientConnect(callback: (peer: WebSocketPeer, headers: { [string]: string }) -> ()) -> WebSocketServer
server:onMessage(callback: (peer: WebSocketPeer, data: string, isBinary: boolean) -> ()) -> WebSocketServer
server:onClientDisconnect(callback: (peer: WebSocketPeer, code: number, reason: string) -> ()) -> WebSocketServer
server:onError(callback: (message: string) -> ()) -> WebSocketServer
```

The server binds to loopback by default. See Security for `host = "0.0.0.0"` LAN exposure.

Peer-to-peer setups run `serve` on one peer and `connect` on the other:

```lua
-- host peer
local server = websocket.serve(7777, { host = "0.0.0.0" })
server:onClientConnect(function(peer)
    peer:send("welcome")
end):onMessage(function(peer, data)
    server:broadcast(data)
end)

-- joining peer
local ws = websocket.connect("ws://192.168.1.5:7777")
```

## WebSocketPeer

```lua
peer:send(data: string) -> (boolean?, string?)
peer:sendBinary(data: string) -> (boolean?, string?)
peer:close(code: number?, reason: string?) -> ()
peer:remoteAddress() -> string? -- "ip:port"
peer:id() -> string?            -- stable per connection, use as table key
```

Peer userdata handed to different callbacks may be distinct Lua values for the same connection.
Use `peer:id()` to key tables.
Methods on a disconnected peer return `nil` and `"websocket peer is disconnected"`.

## Limits

See [Limits and errors](../cpp/limits-and-errors.md) for connection caps and error strings.

## Security

WebSocket traffic is not sandboxed.
Scripts can open outbound client connections to any `ws://` or `wss://` URL the host stack allows.
There is no URL or host allowlist.

The local server binds to loopback (`127.0.0.1`) by default.
Passing `host = "0.0.0.0"` listens on all interfaces, so anything on the LAN can connect.
Only do this intentionally.

TLS on the client:

- `certVerification` defaults to `true` for `wss://`.
- Setting `certVerification = false` disables certificate verification for that connection.
- Use this only for trusted dev servers, for example self-signed certificates.
- On platforms without a usable system certificate store for mbedTLS, pass a PEM bundle via `caBundle`.
- The server side does not support TLS.

For HTTP request caps and TLS options on `geode.utils.web`, see [web](web.md) Security.
See [LuauAPI mod guidelines](../../mod_guidelines.md) for loadstring and network abuse rules.

## Lifecycle

Connections and servers are closed when garbage collected, when `close`/`stop` is called, or when the runtime shuts down.
Nothing closes them when an individual script run returns, so keep a reference for as long as the socket should live.

## Related

- [Getting started overview](../../getting-started/overview.md)
- [Examples](../../getting-started/examples.md)
- [LuauAPI mod guidelines](../../mod_guidelines.md)
- [web](web.md)
- [Tasks and time](tasks.md)
- [Globals](globals.md)
- [Limits and errors](../cpp/limits-and-errors.md)

## Source

- `src/bindings/websocket/WebSocketBinding.cpp`
- `src/bindings/websocket/WebSocketConnection.cpp`
- `src/bindings/websocket/WebSocketServer.cpp`
- `src/bindings/websocket/WebSocketInternal.hpp`
- `tools/luau_codegen/extra_bindings/websocket.dluau`
