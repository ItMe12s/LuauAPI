# modules

## Summary

`require` loads another Luau file as a module.
The system is flat and sandboxed, so you can only load sibling files inside the same resources root.

## How to use it

A module file returns one value, and the caller receives that value.
See [Examples](../../getting-started/examples.md).

## The rules

The rules are strict by design, so loading stays inside the resources root.

- You can only require from a script file. Its chunk name must start with `@`.
- Module names are flat (a single file name with no folders in the path).
- `..` is not allowed, and escaping the root is not allowed.
- Aliases are not supported. A path such as `@alias/name` is rejected.
- The extension must be `.luau`, or absent. When you leave it off, `.luau` is added for you.
- A module must return exactly one value.
- A module must not yield.

Breaking any rule returns an error at load time.

## Limits

Modules share the script size limit and default deadline with entry scripts.

See [Limits and errors](../cpp/limits-and-errors.md).

## What happens on load

The runtime resolves the path, reads the file, compiles bytecode, runs it on a sandboxed thread, and returns the single value.
For the full load pipeline, see [Module system internals](../../contributor/internals/module-system.md).

## Related

- [Globals](globals.md)
- [Sharing APIs between mods](sharing-apis.md)
- [Module system internals](../../contributor/internals/module-system.md)

## Source

- `src/require/Requirer.cpp`
- `src/require/RequirePath.hpp`
- `src/require/PathRules.hpp`
- `src/core/Config.hpp`
