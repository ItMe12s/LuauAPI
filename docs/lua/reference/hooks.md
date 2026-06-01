# Reference: hooks

## Summary

This page is the type reference for the hook API. For usage guidance, see [Hooks](../guide/hooks.md).
The types here match the generated stub `types/geode.d.luau`.

## geode.hook

```lua
geode.hook(target: string, callback: HookCallbackTable) -> HookHandle
```

`geode.hook` registers a callback on a target function and returns a handle.
The target must exist, otherwise the call raises an error. You must pass exactly two arguments.

The target id format is `namespace.Class:method/argCount`, for example `geode.gd.GameManager:setIntGameVariable/2`.

## HookCallbackTable

```lua
type HookCallbackTable = {
    before: ((...any) -> any?)?,
    after: ((...any) -> any?)?,
    priority: number?,
}
```

- `before` runs before the original. It receives `self` and the method arguments.
- `after` runs after the original. It receives `self`, the method arguments, then the return value.
- `priority` orders callbacks. The default is `0`. A lower value runs first for `before`, and a higher value runs first for `after`.

You must provide at least one of `before` or `after`.

## before return values

- Return nothing or `nil` to run the original.
- Return `{ args = {...} }` to replace the method arguments. The list is positional or keyed by argument name.
- Return `geode.skip(value)` to skip the original and use `value` as the return.
- Any other value is ignored, with a logged warning.

## after return values

- Return `nil` to keep the original return.
- Return a value to replace the return.

## geode.skip

```lua
geode.skip(value: any?) -> any
```

Builds a skip marker for use as a `before` return. The original does not run, and the value becomes the return.
Use no argument for functions that return nothing.

## geode.fields

```lua
geode.fields(self: CCNode) -> { [string]: any }
```

Returns a plain table tied to the given node. The table lives as long as the node. The `m_fields` property returns the same table.
The argument must be a `CCNode` or a bound node subtype.

## HookHandle

```lua
type HookHandle = {
    enable: (self: HookHandle) -> (boolean, string?),
    disable: (self: HookHandle) -> (boolean, string?),
    remove: (self: HookHandle) -> boolean,
    isEnabled: (self: HookHandle) -> boolean,
}
```

`enable` and `disable` return success as the first value. On failure they also return an error string as the second value.

## Limits

- Total callbacks across all targets: `4096`.
- Callbacks per target: `64`.
- Callback budget: `50 ms`.

## Source

- `tools/luau_codegen/emit/luau_types/`
- `tools/luau_codegen/emit/cxx_templates.py`
- `tools/luau_codegen/emit/hooks.py`
- `src/lua/Config.hpp`
