# Script events

## Summary

`geode.ScriptEvent` is the built-in event bridge between Luau scripts and C++.
Both directions use string topics and optional string payloads.

Lua `geode.ScriptEvent.post` reaches every C++ listener only.
C++ `imes::luauapi::postScriptEvent` reaches C++ and Lua listeners.
`listen` matches every topic, `listenFor` matches one topic.

## post

```lua
geode.ScriptEvent.post(topic: string, payload: string?) -> ()
```

Posts to C++ `LuaScriptEvent` listeners. The payload defaults to `""`.

```lua
local modId = geode.Mod.getID()

geode.ScriptEvent.post(modId .. "/player.died", "67")
```

## listen

```lua
geode.ScriptEvent.listen(callback: (topic: string, payload: string) -> boolean?, priority: number?) -> ScriptEventListenerHandle
```

Matches every topic.
Callbacks only observe C++ posts, because Lua posts never echo back into Lua listeners.

```lua
local handle = geode.ScriptEvent.listen(function(topic, payload)
    print("script event", topic, payload)
    return false
end)
```

## listenFor

```lua
geode.ScriptEvent.listenFor(topic: string, callback: (topic: string, payload: string) -> boolean?, priority: number?) -> ScriptEventListenerHandle
```

Matches one exact topic. Fires only when a C++ mod posts that topic.

```lua
local modId = geode.Mod.getID()

geode.ScriptEvent.listenFor(modId .. "/game.paused", function(_, payload)
    print("paused by", payload)
end)
```

## Numbers

Convert numbers with `tostring` and `tonumber`.

```lua
local modId = geode.Mod.getID()

geode.ScriptEvent.post(modId .. "/player.jumped", tostring(12))

geode.ScriptEvent.listenFor(modId .. "/player.jumped", function(_, payload)
    local level = tonumber(payload)
    if not level then return end
    print("jumped in level", level)
end)
```

`tonumber` returns `nil` for non-numeric payloads, so guard before use.

## disconnect

```lua
handle:disconnect() -> ()
```

Disconnects a listener handle. Store the handle while the listener is active.
Handles also disconnect during garbage collection and runtime shutdown.

## Naming

Prefix every topic with your mod id and a slash, like node ids.
This is the event version of Geode's `_spr` rule.
See [LuauAPI mod guidelines](../../mod_guidelines.md).

`geode.Mod.getID()` returns your mod id, for example `my.mod-id`.

```lua
local modId = geode.Mod.getID()

geode.ScriptEvent.post(modId .. "/player.jumped", "level-1")
geode.ScriptEvent.listenFor(modId .. "/game.paused", function(_, payload)
    print("paused by", payload)
end)
```

C++ posters use the same prefix with `geode::Mod::get()->getID()`.

## Return value

Return `true` from the callback to stop propagation to later listeners.
Return `false` or nothing to let the event continue.
If a callback errors or times out, LuauAPI logs it and lets propagation continue.

## Example

A C++ mod posts from `include/ScriptEvents.hpp`:

```cpp
auto modId = geode::Mod::get()->getID();
imes::luauapi::postScriptEvent(modId + "/game.paused", "auto");
```

The optional priority argument works like other Geode event listeners.
Higher priority runs first.

## Limits

Posts before the runtime is ready are dropped. Topics and payloads are strings only,
with no history or wildcard matching.

Caps, deadlines, and error strings live in [Limits and errors](../cpp/limits-and-errors.md).

## Related

- [Keyboard input](keyboard-input.md)
- [Mouse input](mouse-input.md)
- [callbacks](callbacks.md)
- [globals](globals.md)
- [Type stubs](type-stubs.md)
- [C++ API reference](../cpp/api-reference.md)
- [Getting started](../../getting-started/overview.md)
- [Sharing APIs between mods](sharing-apis.md)

## Source

- `include/ScriptEvents.hpp`
- `src/bindings/geode/GeodeScriptEventBinding.cpp`
- `src/bindings/geode/ScriptEventInternal.hpp`
- `src/api.cpp`
- `tools/luau_codegen/extra_bindings/scriptevent.dluau`