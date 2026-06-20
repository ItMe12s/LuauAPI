# Module system

## Summary

The module system implements `require`.
It builds on the Luau require runtime with a custom configuration and lives in `src/require/`.
It keeps all loading inside the resources root.

## The requirer

`Requirer` fills a `luarequire_Configuration` with callbacks:

- `is_require_allowed` allows require only from a chunk whose name starts with `@`.
- `reset` moves the current path to the requirer location, validated against the root.
- `to_parent` moves up one level, but never above the root.
- `to_child` moves down one level, only from the root, and only to a flat allowed name.
- `jump_to_alias` is not supported and always reports not found.
- `is_module_present` checks that the resolved file exists.
- `get_chunkname` and `get_loadname` report the chunk name and resolved path.
- `get_cache_key` reads the module file and returns `fileCacheKey(path, contents)`.
- `load` reads, compiles, and runs the module.

## Path rules

Path helpers live in `PathSandbox.hpp`.
User-facing require rules are in [modules](../../reference/lua/modules.md).

- `validateResourcePath()` checks flat names and the `.luau` extension policy.
- `resolveInsideRoot()` canonicalizes a path and enforces root containment.

## Loading a module

`load` runs these steps:

1. Resolve the module path inside the root, or raise an error.
2. Check the file size against the [Limits and errors](../../reference/cpp/limits-and-errors.md).
3. Read the file.
4. Compile to bytecode, using the runtime cache.
5. Create a new thread and sandbox it.
6. Load the bytecode, and apply codegen if enabled.
7. Resume the thread under the default script deadline.
8. Require exactly one return value. Reject a yield, and reject a runtime error.
9. Move the returned value back to the caller.

## File cache keys

`BytecodeCacheKey.hpp` builds cache keys from file metadata and content.
`fileCacheKey` combines path, size, mtime, and content hash.
Both `get_cache_key` and bytecode compilation use it,
so the require and bytecode caches stay in sync when a file or its contents change.

## Sandbox

The path sandbox helpers in `PathSandbox.hpp` read files with a size check
and resolve a candidate path so it stays inside the root.
This is the same containment used by the public `runFile` path.

## Limits

Module size and load deadline match script caps.

See [Limits and errors](../../reference/cpp/limits-and-errors.md).

## Related

- [Architecture](../architecture.md)
- [modules](../../reference/lua/modules.md)
- [Runtime](runtime.md)
- [Limits and errors](../../reference/cpp/limits-and-errors.md)

## Source

- `src/require/Requirer.cpp`
- `src/require/Requirer.hpp`
- `src/require/PathSandbox.hpp`
- `src/require/BytecodeCacheKey.hpp`
