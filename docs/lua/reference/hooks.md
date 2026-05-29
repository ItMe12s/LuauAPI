# Reference: hooks

## Summary

This page is the type reference for the hook API. For usage guidance, see [Hooks and modify](../guide/hooks-and-modify.md). The types here match the generated stub `types/geode.d.luau`.

## geode.hook and geode.modify

```lua
geode.hook(target: string, callback: HookCallbackTable) -> HookHandle
geode.modify(target: string, callback: HookCallbackTable) -> HookHandle
```

`hook` and `modify` are the same function. They register a callback on a target function and return a handle. The target must exist, otherwise the call raises an error. You must pass exactly two arguments.

The target id format is `namespace.Class:method/argCount`, for example `geode.gd.GameManager:setIntGameVariable/2`.

## HookCallbackTable

```lua
type HookCallbackTable = {
    before: ((...any) -> ())?,
    after: ((...any) -> ())?,
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

Builds a skip marker for use as a `before` return. The original does not run, and the value becomes the return. Use no argument for functions that return nothing.

## geode.fields

```lua
geode.fields(self: any) -> { [string]: any }
```

Returns a plain table tied to the given object. The table lives as long as the object. The `m_fields` property returns the same table.

## HookHandle

```lua
type HookHandle = {
    enable: (self: HookHandle) -> boolean,
    disable: (self: HookHandle) -> boolean,
    remove: (self: HookHandle) -> boolean,
    isEnabled: (self: HookHandle) -> boolean,
}
```

## Limits

- Total callbacks across all targets: `4096`.
- Callbacks per target: `64`.
- Callback budget: `50 ms`.

## Source

- `tools/luau_codegen/emit_luau_types.py`
- `tools/luau_codegen/cxx_templates.py`
- `tools/luau_codegen/hooks.py`
- `src/lua/Config.hpp`
