# permission

## Summary

`geode.utils.permission` reads and requests operating system permissions. This matters on mobile.
On Windows and macOS every permission reports granted. On Android and iOS the system decides.
On iOS, `ReadAllFiles` is always denied. Android shows the system dialog for `RecordAudio`.
A bad permission value raises a Lua error. Use the `Permission` table for the values.

| Name           | Value |
| -------------- | ----- |
| `ReadAllFiles` | 3     |
| `RecordAudio`  | 4     |

## getPermissionStatus

```lua
geode.utils.permission.getPermissionStatus(permission: number) -> boolean
```

Returns whether the permission is granted.

## requestPermission

```lua
geode.utils.permission.requestPermission(permission: number, callback: (granted: boolean) -> ()) -> ()
```

Asks the system for the permission. The callback runs later on the main thread with the result.
See [Getting started](../../getting-started/overview.md) for the shared runtime threading rule.

## Permission

```lua
geode.utils.permission.Permission -> { ReadAllFiles: number, RecordAudio: number }
```

A table of the permission values.

## Example

```lua
local perm = geode.utils.permission

if not perm.getPermissionStatus(perm.Permission.RecordAudio) then
    perm.requestPermission(perm.Permission.RecordAudio, function(granted)
        print("granted:", granted)
    end)
end
```

## Related

- [geode.utils](utils.md)
- [callbacks](callbacks.md)
- [globals](globals.md)

## Source

- `src/bindings/geode/GeodeSmallBindings.cpp`
- `tools/luau_codegen/emit/luau_types/manual_fields.py`
