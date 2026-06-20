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
- The require path must start with `./` or `@`.
- Use `./ModuleName` for a sibling module in the same resources root.
- Module names are flat. A single file name with no folders in the path.
- Parent paths such as `../Module` are not supported.
- Escaping the resources root is not allowed.
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
For the full load pipeline, see [Module system](../../contributor/internals/module-system.md).

## Related

- [Getting started](../../getting-started/overview.md)
- [Examples](../../getting-started/examples.md)
- [globals](globals.md)
- [sharing APIs between mods](sharing-apis.md)
- [Module system](../../contributor/internals/module-system.md)

## Source

- `src/require/Requirer.cpp`
- `src/require/PathSandbox.hpp`
- `src/core/Config.hpp`
